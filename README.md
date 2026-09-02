http.c: A very basic HTTP server application 

TODO:
Off-by-one error in parse_get_req,
recv buffer size in step 2c,
perror vs fprintf distinction,
missing close(new_fd) in continue branches,
bind error handling,
free chain and triple fopen
restructure the input side (req parameter + function separation)

long-term goals:
add multithreading
receive and parse other HTTP methods (POST, PUT, PATCH, DELETE)
HTTP/2 support
keep-alive support
cookies & session management
some security measures such as preventing path traversal attacks