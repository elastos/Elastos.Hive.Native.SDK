Elastos Hive core APIs
======================

Constants
---------

HIVE_MAX_USER_ID_LEN
####################

.. doxygendefine:: HIVE_MAX_USER_ID_LEN
   :project: HiveAPI

HIVE_MAX_USER_NAME_LEN
######################

.. doxygendefine:: HIVE_MAX_USER_NAME_LEN
   :project: HiveAPI

HIVE_MAX_PHONE_LEN
##################

.. doxygendefine:: HIVE_MAX_PHONE_LEN
   :project: HiveAPI

HIVE_MAX_EMAIL_LEN
##################

.. doxygendefine:: HIVE_MAX_EMAIL_LEN
   :project: HiveAPI

HIVE_MAX_REGION_LEN
###################

.. doxygendefine:: HIVE_MAX_REGION_LEN
   :project: HiveAPI

HIVE_MAX_DRIVE_ID_LEN
#####################

.. doxygendefine:: HIVE_MAX_DRIVE_ID_LEN
   :project: HiveAPI

HIVE_MAX_FILE_ID_LEN
####################

.. doxygendefine:: HIVE_MAX_FILE_ID_LEN
   :project: HiveAPI

HIVE_MAX_FILE_TYPE_LEN
######################

.. doxygendefine:: HIVE_MAX_FILE_TYPE_LEN
   :project: HiveAPI

Data types
----------

ElaLogLevel
###########

.. doxygenenum:: ElaLogLevel
   :project: HiveAPI

HiveDriveType
#############

.. doxygenenum:: HiveDriveType
   :project: HiveAPI

HiveOptions
###########

.. doxygenstruct:: HiveOptions
   :project: HiveAPI
   :members:

OneDriveOptions
###############

.. doxygenstruct:: OneDriveOptions
   :project: HiveAPI
   :members:

HiveRpcNode
###########

.. doxygenstruct:: HiveRpcNode
   :project: HiveAPI
   :members:

IPFSOptions
###########

.. doxygenstruct:: IPFSOptions
   :project: HiveAPI
   :members:

HiveClientInfo
##############

.. doxygenstruct:: HiveClientInfo
   :project: HiveAPI
   :members:

HiveDriveInfo
#############

.. doxygenstruct:: HiveDriveInfo
   :project: HiveAPI
   :members:

HiveFileInfo
############

.. doxygenstruct:: HiveFileInfo
   :project: HiveAPI
   :members:

HiveRequestAuthenticationCallback
#################################

.. doxygentypedef:: HiveRequestAuthenticationCallback
   :project: HiveAPI

KeyValue
########

.. doxygenstruct:: KeyValue
   :project: HiveAPI
   :members:

HiveFilesIterateCallback
########################

.. doxygentypedef:: HiveFilesIterateCallback
   :project: HiveAPI

Whence
######

.. doxygenenum:: Whence
   :project: HiveAPI

Functions
---------

Client instance functions
#########################

hive_client_new
~~~~~~~~~~~~~~~

.. doxygenfunction:: hive_client_new
   :project: HiveAPI

hive_cient_close
~~~~~~~~~~~~~~~~

.. doxygenfunction:: hive_client_close
   :project: HiveAPI

hive_client_login
~~~~~~~~~~~~~~~~~

.. doxygenfunction:: hive_client_login
   :project: HiveAPI

hive_client_logout
~~~~~~~~~~~~~~~~~~

.. doxygenfunction:: hive_client_logout
   :project: HiveAPI

hive_client_get_info
~~~~~~~~~~~~~~~~~~~~

.. doxygenfunction:: hive_client_get_info
   :project: HiveAPI

Drive instance functions
########################

hive_drive_open
~~~~~~~~~~~~~~~

.. doxygenfunction:: hive_drive_open
   :project: HiveAPI

hive_drive_close
~~~~~~~~~~~~~~~~

.. doxygenfunction:: hive_drive_close
   :project: HiveAPI

hive_drive_get_info
~~~~~~~~~~~~~~~~~~~

.. doxygenfunction:: hive_drive_get_info
   :project: HiveAPI

hive_drive_list_files
~~~~~~~~~~~~~~~~~~~~~

.. doxygenfunction:: hive_drive_list_files
   :project: HiveAPI

hive_drive_mkdir
~~~~~~~~~~~~~~~~

.. doxygenfunction:: hive_drive_mkdir
   :project: HiveAPI

hive_drive_move_file
~~~~~~~~~~~~~~~~~~~~

.. doxygenfunction:: hive_drive_move_file
   :project: HiveAPI

hive_drive_copy_file
~~~~~~~~~~~~~~~~~~~~

.. doxygenfunction:: hive_drive_copy_file
   :project: HiveAPI

hive_drive_delete_file
~~~~~~~~~~~~~~~~~~~~~~

.. doxygenfunction:: hive_drive_delete_file
   :project: HiveAPI

hive_drive_file_stat
~~~~~~~~~~~~~~~~~~~~

.. doxygenfunction:: hive_drive_file_stat
   :project: HiveAPI

File instance functions
#######################

hive_file_open
~~~~~~~~~~~~~~

.. doxygenfunction:: hive_file_open
   :project: HiveAPI

hive_file_close
~~~~~~~~~~~~~~~

.. doxygenfunction:: hive_file_close
   :project: HiveAPI

hive_file_seek
~~~~~~~~~~~~~~

.. doxygenfunction:: hive_file_seek
   :project: HiveAPI

hive_file_read
~~~~~~~~~~~~~~

.. doxygenfunction:: hive_file_read
   :project: HiveAPI

hive_file_write
~~~~~~~~~~~~~~~

.. doxygenfunction:: hive_file_write
   :project: HiveAPI

hive_file_commit
~~~~~~~~~~~~~~~~

.. doxygenfunction:: hive_file_commit
   :project: HiveAPI

hive_file_discard
~~~~~~~~~~~~~~~~~

.. doxygenfunction:: hive_file_discard
   :project: HiveAPI

Utility functions
#################

ela_log_init
~~~~~~~~~~~~

.. doxygenfunction:: ela_log_init
   :project: HiveAPI

hive_get_error
~~~~~~~~~~~~~~

.. doxygenfunction:: hive_get_error
   :project: HiveAPI

hive_clear_error
~~~~~~~~~~~~~~~~

.. doxygenfunction:: hive_clear_error
   :project: HiveAPI

hive_get_strerror
~~~~~~~~~~~~~~~~~

.. doxygenfunction:: hive_get_strerror
   :project: HiveAPI
