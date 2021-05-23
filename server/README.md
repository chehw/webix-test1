Dependencies
-------------

These dependencies are required:

----------------------------------------
Library         |  Description
----------------|-----------------------
libdb-5.3       | NoSQL DB
libsoup-2.4     | http server
libjwt          | json web token
libjson-c       | json
libuuid         | uuid
----------------------------------------

### install dependencies

```
$ sudo apt-get update
$ sudo apt-get install build-essential 
$ sudo apt-get install libdb5.3-dev libsoup2.4-dev libjwt-dev libjson-c-dev uuid-dev 
```

### build

```
$ cd {project_dir}/server
$ make
```
