## Thinking of Hive Sotorage APIs

### 1. API version
- version

### 2. Standard methods 
- Stat ( including timestamp)
- SetTimeStamp
- List
- Mkdir
- Move
- Copy
- Delete
- Read (Get/Cat)
- Write (Put)

### 3. Synchronization of Contents (exclusive)
- Sync 

### 4. Sharing resources (exclusive)
- Lock
- Unlock
  
### 5. Retrival 
- Search

### 6. Content Versioning (Not this version)
- List versions
- Version
- Restore version

## References

###1. OneDrive
- stat  — Get a drive-item resource.
- settimestamp — Update derive-item properties
- list — List children of derive item;
- mkdir — Create a new folder
- move — move a drive-item to new folder;
- copy — copy a drive-item
- delete — Delete a drive-item
- read — Download a file in specific format;
- write —  Upload or replace the content of drive-item

### 2. WebDAV
- stat — PROPFIND 
- settimestamp — PROPATCH
- list — PROPFIND
- mkdir  — MKCOL
- move — MOVE
- copy — COPY
- delete — (Http Method/DELETE /resource path) 
- read (Http Method/GET/resource path)
- write (Http Method/POST/resource path)

### 3. Hive Cluster
- stat —  /api/v0/files/stat
- settimestamp — Not supported.
- list —/api/v0/files/ls
- mkdir — /api/v0/files/mkdir
- move — /api/v0/files/mv
- copy — /api/v0/files/cp
- delete — /api/v0/files/rm 
- read — /api/v0/files/read
- write — /api/v0/files/write

### 4. Local Storage (TODO)