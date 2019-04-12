#!/bin/bash
# Written by Wang Yi (<wangyi@storswift.com>), Wan Lei (<wanlei@storswift.com>), and Leng Bo (<lengbo@storswift.com>).

# Uncomment the two lines to use your own fixed UID
# HIVE_UID=uid-f409f8e4-667f-4f55-9f4c-3b129375a663
# HIVE_PEERID=QmZP7ahpnbyUyhToA2h1rJB6M5VXYcqscye8n1Rpr3ZN3b

# You can set the log filenames. If not, the script will generate them.
# logfile=dstore_test.log
# statisticsfile=dstore_test.sta

if [ "${logfile}" = "" ]; then
    logfile=SERVER-$1-`date "+%Y-%m-%d_%H:%M"`.log
fi
if [ "${statisticsfile}" = "" ]; then
    statisticsfile=SERVER-$1-`date "+%Y-%m-%d_%H:%M"`.sta
fi

function show_error()
{
    logs "[Error] $1"
    exit 2
}

function show_steps()
{
    echo
    echo "************************************************************"
    echo $1
    echo "************************************************************"
}

function logs_time()
{
    start=$2
    end=$3
    start_s=$(echo ${start} | cut -d '.' -f 1)
    start_ns=$(echo ${start} | cut -d '.' -f 2)
    end_s=$(echo ${end} | cut -d '.' -f 1)
    end_ns=$(echo ${end} | cut -d '.' -f 2)
    time_ns=$(( ( 10#${end_s} - 10#${start_s} ) * 1000 + ( 10#${end_ns} / 1000000 - 10#${start_ns} / 1000000 ) ))
    echo "[$1] Begin:`date -d @${start_s} +%H:%M:%S`, End:`date -d @${end_s} +%H:%M:%S`. Last ${time_ns} ms" \
        | tee -a $logfile $statisticsfile
}

function logs()
{
    echo [ `date "+%Y-%m-%d %H:%M:%S"`] $1 >>$logfile
}


# send message / add k-v
# ussage: ./cpp-test/dstore_test send <host> <uid> <key> <value>

function msg_send {
    host=$1
    uid=$2
    key=$3
    value=$4
    # echo dstore_test send ${host} ${uid} ${key} ${value}
    dstore_test send ${host} ${uid} ${key} ${value}
}

# read message / read value by key
# usage: ./cpp-test/dstore_test read <host> <peerId> <key>

function msg_read {
    host=$1
    peerid=$2
    key=$3
    # echo dstore_test read ${host} ${peerid} ${key}
    dstore_test read ${host} ${peerid} ${key}
}

# remove messsage / remove k and its values
# usage: ./cpp-test/dstore_test remove <host> <uid> <key>
function msg_remove {
    host=$1
    uid=$2
    key=$3
    # echo dstore_test remove ${host} ${uid} ${key}
    dstore_test remove ${host} ${uid} ${key}
}

if [ $# -lt 2 ]; then
    echo "Please set the IP addresses of two servers!"
    echo " $0 SERVER1 SERVER2"
    echo "Example:"
    echo " $0 192.168.0.2 192.168.0.3"
    echo "    # It will test send key-value to 192.168.0.2, and read this key-value from 192.168.0.3."
    exit 1
fi

HIVE_HOST1=$1
HIVE_HOST2=$2

if [ "${HIVE_UID}" = "" ] && [ "${HIVE_PEERID}" = "" ]; then
    temp=`curl http://${HIVE_HOST1}:9095/api/v0/uid/new 2>/dev/null` \
        || show_error "Can not new a UID on ${HIVE_HOST1}"
    HIVE_UID=`echo ${temp} | jq .UID | sed s/\"//g`
    HIVE_PEERID=`echo ${temp} | jq .PeerID | sed s/\"//g`
fi

echo "===============================================================================================" \
    | tee -a ${logfile} ${statisticsfile}
echo "******************** Send key-value to ${HIVE_HOST1}. Read from ${HIVE_HOST2}*****************"  \
    | tee -a ${logfile} ${statisticsfile}
echo "===============================================================================================" \
    | tee -a ${logfile} ${statisticsfile}
echo 
echo HIVE_UID=${HIVE_UID}
echo HIVE_PEERID=${HIVE_PEERID}
logs "[Info] Begin to  send key-value to ${HIVE_HOST1}"


TEST_KEY=test-key-${HIVE_HOST2}
TEST_VALUE=test-key-${HIVE_HOST2}-item001

time_start_send=`date +%s.%N`
msg_send ${HIVE_HOST1} ${HIVE_UID} ${TEST_KEY} ${TEST_VALUE}  \
    || show_error "Cannot send key-value to ${HIVE_HOST1}."
# sleep 3
time_end_send=`date +%s.%N`
logs "[Info] Now send key-value to ${HIVE_HOST1} successfully."
i=1
res=""
while :
do
    res=`msg_read ${HIVE_HOST2} ${HIVE_PEERID} ${TEST_KEY}`
    echo "****************************************"
    echo "[Info] Key:${TEST_KEY}, Raw Value:"
    echo ${res}
    echo "****************************************"
    res3=`echo ${res} | awk '{print $7}'`
    if [ x"${res3}" = x"${TEST_VALUE}" ];then
        time_end_read=`date +%s.%N`
        logs "[Info] Now read key-value from ${HIVE_HOST2} successfully." 
        break
    fi
    echo "[Info] Key:${TEST_KEY}, Value: ${res3}. But the value should be ${TEST_VALUE}. Wait 1 second."
    sleep 1
    ((i++))
done
msg_remove ${HIVE_HOST1} ${HIVE_UID} ${TEST_KEY}
logs_time Send_K-V_To_${HIVE_HOST1} ${time_start_send} ${time_end_send}
logs_time Read_K-V_From_${HIVE_HOST2} ${time_end_send} ${time_end_read}
echo "" >>$statisticsfile 

sleep 3

echo "Test done!"
