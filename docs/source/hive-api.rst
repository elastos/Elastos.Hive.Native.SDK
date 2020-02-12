Elastos Hive core APIs
======================

Constants
---------

HIVE_MAX_VALUE_LEN
##################

.. doxygendefine:: HIVE_MAX_VALUE_LEN
   :project: HiveAPI

HIVE_MAX_FILE_SIZE
##################

.. doxygendefine:: HIVE_MAX_FILE_SIZE
   :project: HiveAPI

HIVE_MAX_IPFS_CID_LEN
#####################

.. doxygendefine:: HIVE_MAX_IPFS_CID_LEN
   :project: HiveAPI

Data types
----------

HiveLogLevel
############

.. doxygenenum:: HiveLogLevel
   :project: HiveAPI

HiveBackendType
###############

.. doxygenenum:: HiveBackendType
   :project: HiveAPI

HiveOptions
###########

.. doxygenstruct:: HiveOptions
   :project: HiveAPI
   :members:

HiveConnectOptions
##################

.. doxygenstruct:: HiveConnectOptions
   :project: HiveAPI
   :members:

HiveRequestAuthenticationCallback
#################################

.. doxygentypedef:: HiveRequestAuthenticationCallback
   :project: HiveAPI

OneDriveConnectOptions
######################

.. doxygenstruct:: OneDriveConnectOptions
   :project: HiveAPI
   :members:

IPFSNode
########

.. doxygenstruct:: IPFSNode
   :project: HiveAPI
   :members:

IPFSConnectOptions
##################

.. doxygenstruct:: IPFSConnectOptions
   :project: HiveAPI
   :members:

HiveFilesIterateCallback
########################

.. doxygentypedef:: HiveFilesIterateCallback
   :project: HiveAPI

IPFSCid
#######

.. doxygenstruct:: IPFSCid
   :project: HiveAPI
   :members:

HiveKeyValuesIterateCallback
############################

.. doxygentypedef:: HiveKeyValuesIterateCallback
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

Connect instance functions
##########################

hive_client_connect
~~~~~~~~~~~~~~~~~~~

.. doxygenfunction:: hive_client_connect
   :project: HiveAPI

hive_client_disconnect
~~~~~~~~~~~~~~~~~~~~~~

.. doxygenfunction:: hive_client_disconnect
   :project: HiveAPI

hive_set_encrypt_key
~~~~~~~~~~~~~~~~~~~~

.. doxygenfunction:: hive_set_encrypt_key
   :project: HiveAPI

hive_put_file
~~~~~~~~~~~~~

.. doxygenfunction:: hive_put_file
   :project: HiveAPI

hive_put_file_from_buffer
~~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenfunction:: hive_put_file_from_buffer
   :project: HiveAPI

hive_get_file_length
~~~~~~~~~~~~~~~~~~~~

.. doxygenfunction:: hive_get_file_length
   :project: HiveAPI

hive_get_file_to_buffer
~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenfunction:: hive_get_file_to_buffer
   :project: HiveAPI

hive_get_file
~~~~~~~~~~~~~

.. doxygenfunction:: hive_get_file
   :project: HiveAPI

hive_delete_file
~~~~~~~~~~~~~~~~

.. doxygenfunction:: hive_delete_file
   :project: HiveAPI

hive_list_files
~~~~~~~~~~~~~~~

.. doxygenfunction:: hive_list_files
   :project: HiveAPI

hive_ipfs_put_file
~~~~~~~~~~~~~~~~~~

.. doxygenfunction:: hive_ipfs_put_file
   :project: HiveAPI

hive_ipfs_put_file_from_buffer
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenfunction:: hive_ipfs_put_file_from_buffer
   :project: HiveAPI

hive_ipfs_get_file_length
~~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenfunction:: hive_ipfs_get_file_length
   :project: HiveAPI

hive_ipfs_get_file_to_buffer
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenfunction:: hive_ipfs_get_file_to_buffer
   :project: HiveAPI

hive_ipfs_get_file
~~~~~~~~~~~~~~~~~~

.. doxygenfunction:: hive_ipfs_get_file
   :project: HiveAPI

hive_put_value
~~~~~~~~~~~~~~

.. doxygenfunction:: hive_put_value
   :project: HiveAPI

hive_set_value
~~~~~~~~~~~~~~

.. doxygenfunction:: hive_set_value
   :project: HiveAPI

hive_get_values
~~~~~~~~~~~~~~~

.. doxygenfunction:: hive_get_values
   :project: HiveAPI

hive_delete_key
~~~~~~~~~~~~~~~

.. doxygenfunction:: hive_delete_key
   :project: HiveAPI

Utility functions
#################

hive_log_init
~~~~~~~~~~~~~

.. doxygenfunction:: hive_log_init
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
