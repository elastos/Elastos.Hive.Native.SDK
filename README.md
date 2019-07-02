[![Build Status](https://travis-ci.org/elastos/Elastos.NET.Hive.Native.SDK.svg)](https://travis-ci.org/elastos/Elastos.NET.Hive.Native.SDK)

# Overview
Elastos.NET.Hive.Native.SDK provides a set of APIs to enable SDK users to perform file 
operations on various storage backends(e.g., OneDrive, IPFS, DropBox, WebDAV, etc..., 
only OneDrive and IPFS are implemented currently) in a unified manner.

# Concepts
## Client
Client is an abstract class representing an application acting on behalf of a specific 
user of a specific storage backend(OneDrive, IPFS, native file system, etc...). In another 
word, the client acts as the associated user. In OneDrive's case, the client works in 
delegated mode. There is one client deriving class for each backend.
## Drive 
Drive is an abstract class generated by a client. It represents the default drive of the 
client's associating user. There is one drive deriving class for each backend.
## File
File is an abstract class generated by a drive. It represents a file under the drive.
There is one client deriving class for each backend.

# Use of Elastos.NET.Hive.Native.SDK
## Client APIs
- A list of client APIs:
  - hive_client_new()
  - hive_client_close()
  - hive_client_login()
  - hive_client_logout()
  - hive_client_get_info()
- Client APIs are a set of abstract APIs of client class. They can be used by passing an
instance of a client deriving class. Constructing an instance of a specific client deriving 
class by passing corresponding options to hive_client_new().
- A client can be in one of two states: "raw" or "logined". When a client is in "raw"
state, no user is associated with it. When a client is in "logined" state, a user is 
associated with it. When a client is generated by calling hive_client_new(), it's in state 
"raw". Subsequent successful call of hive_client_login() changes the client's state to 
"logined". Subsequent successful call of hive_client_logout() reverts the client's state to 
"raw". Calling hive_client_get_info() is only allowed when the client is in "logined" state, 
and the client's state remains unchanged.
## Drive APIs 
- A list of drive APIs:
  - hive_drive_open()
  - hive_drive_get_info()
  - hive_drive_list_files()
  - hive_drive_file_stat()
  - hive_drive_mkdir()
  - hive_drive_move_file()
  - hive_drive_copy_file()
  - hive_drive_delete_file()
  - hive_drive_close()
- Drive APIs are a set of abstract APIs of drive class. They can be used by passing an
instance of a drive deriving class. Constructing an instance of a specific drive deriving 
class by passing corresponding instance of client deriving class to hive_drive_open().
- These APIs are only allowed to be called when the client instance generating the drive
instance is in "logined" state.
## File APIs
- A list of file APIs:
  - hive_file_open()
  - hive_file_close()
  - hive_file_seek()
  - hive_file_read()
  - hive_file_write()
  - hive_file_commit()
  - hive_file_discard()
- File APIs are a set of abstract APIs of file class. They can be used by passing an
instance of a file deriving class. Constructing an instance of a specific file deriving 
class by passing corresponding instance of drive deriving class to hive_file_open(). The file
APIs mimic the unix file I/O APIs.
- These APIs are only allowed to be called when the client instance generating the drive
instance generating the file instance is in "logined" state.
- For those backends which do not support incremental write directly, a workaround is taken:
first download the whole file from backend to a local copy, and subsequent write is done
on the local copy, then upload the modified file to backend finally(hive_file_commit()
does the trick; hive_file_discard() discards the local changes). hive_file_commit() and
hive_file_discard() are only available on these very backends.
## Example
```c
    OneDriveOptions options = {
        .base.drive_type = HiveDriveType_OneDrive,
        .base.persistent_location = ".",
        .client_id = "your-client-id",
        .scope = "your-scope",
        .redirect_url = "your-redirect-url"
    };
    HiveClient *client;
    HiveDrive *drive;
    HiveFile *file;
    char strerr_buf[1024];
    int rc;

    /* construct a OneDrive client instance */
    client = hive_client_new((HiveOptions *)&options);
    if (!client) {
        fprintf(stderr, "construct client instance failure: %s\n", 
                hive_get_strerror(hive_get_error(), strerr_buf, sizeof(strerr_buf)));
        return -1;
    }
    
    /* login a user to OneDrive */
    rc = hive_client_login(client, open_url_cb, NULL);
    if (rc < 0) {
        fprintf(stderr, "client instance login failure: %s\n", 
                hive_get_strerror(hive_get_error(), strerr_buf, sizeof(strerr_buf)));
        return -1;
    }
    
    /* construct a OneDrive drive instance from client */
    drive = hive_drive_open(client);
    if (!drive) {
        fprintf(stderr, construct drive instance failure: "%s\n", 
                hive_get_strerror(hive_get_error(), strerr_buf, sizeof(strerr_buf)));
        return -1;
    }
    
    /* create a directory under /foo in default drive of the user */ 
    rc = hive_drive_mkdir(drive, "/foo");
    if (rc < 0) {
        fprintf(stderr, "mkdir /foo failure: %s\n", 
                hive_get_strerror(hive_get_error(), strerr_buf, sizeof(strerr_buf)));
        return -1;
    }
    
    /* open file /foo/bar.txt in default drive of the usr */
    file = hive_file_open(drive, "/foo/bar.txt", "w+");
    if (!file) {
        fprintf(stderr, construct client instance failure: "%s\n", 
                hive_get_strerror(hive_get_error(), strerr_buf, sizeof(strerr_buf)));
        return -1;
    }
    
    /* write to local copy */ 
    rc = hive_file_write(file, "hello world!!!", sizeof("hello world!!!"));
    if (rc < 0) {
        fprintf(stderr, write to file failure: "%s\n", 
                hive_get_strerror(hive_get_error(), strerr_buf, sizeof(strerr_buf)));
        return -1;
    }
    
    /* upload to backend */
    rc = hive_file_commit(file);
    if (rc < 0) {
        fprintf(stderr, commit file failure: "%s\n", 
                hive_get_strerror(hive_get_error(), strerr_buf, sizeof(strerr_buf)));
        return -1;
    }
    
    hive_file_close(file);
    hive_drive_close(drive);
    hive_client_close(client);
    return 0;
```
