Steps for interacting with the API:
1. Make an http GET request to TheCube.local (or localhost) on port 55200.
2. Wait for this request to return code 200 OK.
3. Interact with the API at TheCube.local:55280, localhost:55280, or the unix socket.

The API is a RESTful API, and the following endpoints are available:
- GET / - Main GUI page
- GET /auth.js - Authentication Javascript functions
- GET /getEndpoints - Returns a list of all available endpoints in JSON format
- GET /getCubeSocketPath - Returns the path to the cube socket in JSON format
