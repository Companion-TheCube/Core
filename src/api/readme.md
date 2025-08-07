Steps for interacting with the API:
1. Make an http GET request to TheCube.local (or localhost) on port 55200.
2. Wait for this request to return code 200 OK.
3. Interact with the API at TheCube.local:55280, localhost:55280, or the unix socket.

The API is a RESTful API, and the following endpoints are available:
- GET / - Main GUI page
- GET /auth.js - Authentication Javascript functions
- GET /getEndpoints - Returns a list of all available endpoints in JSON format
- GET /getCubeSocketPath - Returns the path to the cube socket in JSON format

## Additional Endpoints

Interface endpoints are exposed at `/InterfaceName-endpoint`.

### AudioManager
- **GET /AudioManager-start** – Start the audio. (Public)
- **GET /AudioManager-stop** – Stop the audio. (Public)
- **GET /AudioManager-toggleSound** – Toggle the sound. (Public)
- **GET /AudioManager-setSound** – Set the sound state; accepts `soundOn=true|false`. (Public)

### CubeAuth
- **GET /CubeAuth-authHeader** – Authorize a client and return an authentication header; requires `client_id` and `initial_code`. (Public)
- **GET /CubeAuth-initCode** – Generate an initial authorization code for a client; requires `client_id`. (Public)

### CubeDB
- **POST /CubeDB-saveBlob** – Save a blob for a client or app; requires `client_id` or `app_id` and either `stringBlob` or `blob` (base64). (Private)
- **GET /CubeDB-insertData** – Insert test data into the database. (Public)
- **GET /CubeDB-retrieveBlobBinary** – Retrieve a binary blob; parameters `clientOrApp_id` and `blob_id`; returns base64 data. (Private)
- **GET /CubeDB-retrieveBlobString** – Retrieve a string blob; parameters `clientOrApp_id` and `blob_id`. (Private)

### Bluetooth
- **GET /Bluetooth-stopBTManager** – Stop the Bluetooth manager. (Private)
- **GET /Bluetooth-startBTManager** – Start the Bluetooth manager. (Private)
- **POST /Bluetooth-addBTService** – Add a Bluetooth service to the manager. (Private)

### GUI
- **GET /GUI-messageBox** – Display a message box; parameters `text` and `title`. (Public)
- **GET /GUI-textBox** – Display a text box; parameters `text`, `title`, `size-x`, `size-y`, `position-x`, `position-y`. (Public)
- **POST /GUI-addMenu** – Add a menu to the GUI. (Private)

### Notifications
- **POST /Notifications-showNotificationOkayWarningError** – Show a notification with an optional callback. (Private)
- **POST /Notifications-showNotificationYesNo** – Show a yes/no notification with callbacks. (Private)

### PersonalityManager
- **GET /PersonalityManager-getEmotionValue** – Retrieve the value of an emotion; requires `emotion`. (Private)
- **GET /PersonalityManager-setEmotionValue** – Set an emotion value with optional targeting parameters; requires `emotion` and may include `value`, `targetValue`, `targetTime`, `expiration`, `rampType`. (Private)

### Logger
- **POST /Logger-log** – Log a message; requires `message`, `level`, `source`, `line`, and `function`. (Private)
- **GET /Logger-getLogs** – Retrieve log entries. (Private)
