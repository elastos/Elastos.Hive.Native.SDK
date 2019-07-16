Elastos Hive error number
=========================

Error number format
-------------------

The Hive error numbering space is extensible. The numbering space has the following internal structure.

+-----------+-----------+--------+
|   0 ~ 3   |   4 ~ 7   | 8 ~ 31 |
+===========+===========+========+
| Severity  | Facility  |  Code  |
+-----------+-----------+--------+

An error number value has 32 bits divided into three fields: a severity code, a facility code, and an error code. The severity code indicates whether the return value represents information, warning, or error. The facility code identifies the area of the system responsible for the error. The error code is a unique number that is assigned to represent the exception.

Format details
##############

* Severity - indicates a error

  - 1 - Failure

* Facility - indicates the Carrier module that is responsible for the error. Available facility codes are shown below:

  - 1 - General
  - 2 - System
  - 3 - Reserved/not used
  - 4 - Curl
  - 5 - Curlu
  - 6 - HTTP status

* Code - is the facility's error code

Example
#######

0x81000001

* 0x8 - Error
* 0x1 - General
* 0x1 - The error number(HIVEERR_INVALID_ARGS)

Error codes
-----------

HIVEOK
######

.. doxygendefine:: HIVEOK
   :project: HiveAPI

HIVEERR_INVALID_ARGS
####################

.. doxygendefine:: HIVEERR_INVALID_ARGS
   :project: HiveAPI


HIVEERR_OUT_OF_MEMORY
#####################

.. doxygendefine:: HIVEERR_OUT_OF_MEMORY
   :project: HiveAPI

HIVEERR_BUFFER_TOO_SMALL
########################

.. doxygendefine:: HIVEERR_BUFFER_TOO_SMALL
   :project: HiveAPI

HIVEERR_BAD_PERSISTENT_DATA
###########################

.. doxygendefine:: HIVEERR_BAD_PERSISTENT_DATA
   :project: HiveAPI

HIVEERR_INVALID_PERSISTENCE_FILE
################################

.. doxygendefine:: HIVEERR_INVALID_PERSISTENCE_FILE
   :project: HiveAPI

HIVEERR_INVALID_CREDENTIAL
##########################

.. doxygendefine:: HIVEERR_INVALID_CREDENTIAL
   :project: HiveAPI

HIVEERR_NOT_READY
#################

.. doxygendefine:: HIVEERR_NOT_READY
   :project: HiveAPI

HIVEERR_NOT_EXIST
#################

.. doxygendefine:: HIVEERR_NOT_EXIST
   :project: HiveAPI

HIVEERR_ALREADY_EXIST
#####################

.. doxygendefine:: HIVEERR_ALREADY_EXIST
   :project: HiveAPI

HIVEERR_INVALID_USERID
######################

.. doxygendefine:: HIVEERR_INVALID_USERID
   :project: HiveAPI

HIVEERR_WRONG_STATE
###################

.. doxygendefine:: HIVEERR_WRONG_STATE
   :project: HiveAPI

HIVEERR_BUSY
############

.. doxygendefine:: HIVEERR_BUSY
   :project: HiveAPI

HIVEERR_LANGUAGE_BINDING
########################

.. doxygendefine:: HIVEERR_LANGUAGE_BINDING
   :project: HiveAPI

HIVEERR_ENCRYPT
###############

.. doxygendefine:: HIVEERR_ENCRYPT
   :project: HiveAPI

HIVEERR_NOT_IMPLEMENTED
#######################

.. doxygendefine:: HIVEERR_NOT_IMPLEMENTED
   :project: HiveAPI

HIVEERR_NOT_SUPPORTED
#####################

.. doxygendefine:: HIVEERR_NOT_SUPPORTED
   :project: HiveAPI

HIVEERR_LIMIT_EXCEEDED
######################

.. doxygendefine:: HIVEERR_LIMIT_EXCEEDED
   :project: HiveAPI

HIVEERR_ENCRYPTED_PERSISTENT_DATA
#################################

.. doxygendefine:: HIVEERR_ENCRYPTED_PERSISTENT_DATA
   :project: HiveAPI

HIVEERR_BAD_BOOTSTRAP_HOST
##########################

.. doxygendefine:: HIVEERR_BAD_BOOTSTRAP_HOST
   :project: HiveAPI

HIVEERR_BAD_BOOTSTRAP_PORT
##########################

.. doxygendefine:: HIVEERR_BAD_BOOTSTRAP_PORT
   :project: HiveAPI

HIVEERR_BAD_ADDRESS
###################

.. doxygendefine:: HIVEERR_BAD_ADDRESS
   :project: HiveAPI

HIVEERR_BAD_JSON_FORMAT
#######################

.. doxygendefine:: HIVEERR_BAD_JSON_FORMAT
   :project: HiveAPI

HIVEERR_TRY_AGAIN
#################

.. doxygendefine:: HIVEERR_TRY_AGAIN
   :project: HiveAPI

HIVEERR_UNKNOWN
###############

.. doxygendefine:: HIVEERR_UNKNOWN
   :project: HiveAPI
