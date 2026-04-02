let test = "nothing";
let authToken = null;
let clientId = "webui"; // default; user can override below
let pendingCode = null;

function describeParamType(schema) {
    if (!schema || typeof schema !== "object") {
        return "string";
    }
    if (Array.isArray(schema.type)) {
        return schema.type.join("|");
    }
    return schema.type || "string";
}

function extractParams(paramsSchema) {
    if (Array.isArray(paramsSchema)) {
        return paramsSchema.map((name) => ({
            name,
            required: false,
            schema: { type: "string" }
        }));
    }

    if (!paramsSchema || typeof paramsSchema !== "object") {
        return [];
    }

    const properties = paramsSchema.properties && typeof paramsSchema.properties === "object"
        ? paramsSchema.properties
        : {};
    const requiredFields = new Set(Array.isArray(paramsSchema.required) ? paramsSchema.required : []);

    return Object.entries(properties).map(([name, schema]) => ({
        name,
        required: requiredFields.has(name),
        schema: schema || { type: "string" }
    }));
}

function renderEndpointLoadError(message) {
    const endpoints = document.getElementById("endpoints");
    if (!endpoints) {
        return;
    }
    endpoints.replaceChildren();
    const error = document.createElement("p");
    error.style.color = "#b00020";
    error.textContent = message;
    endpoints.appendChild(error);
}

function getCookie(name) {
    const match = document.cookie.match(new RegExp("(?:^|; )" + name.replace(/([.$?*|{}()\[\]\\/+^])/g, "\\$1") + "=([^;]*)"));
    return match ? decodeURIComponent(match[1]) : null;
}

function setCookie(name, value, days) {
    const expires = days ? "; expires=" + new Date(Date.now() + days * 864e5).toUTCString() : "";
    document.cookie = name + "=" + encodeURIComponent(value) + expires + "; path=/";
}

authToken = getCookie("cube_auth") || null;
console.log("auth.js is loaded");

// Build simple auth UI
const toolbar = document.getElementById("toolbar") || (function(){
    const t = document.createElement("div");
    t.id = "toolbar";
    document.body.prepend(t);
    return t;
})();

const clientIdInput = document.createElement("input");
clientIdInput.type = "text";
clientIdInput.placeholder = "client_id (e.g., webui)";
clientIdInput.value = clientId;
toolbar.appendChild(clientIdInput);

const returnCodeCheckbox = document.createElement("input");
returnCodeCheckbox.type = "checkbox";
returnCodeCheckbox.id = "returnCodeCheckbox";
const returnCodeLabel = document.createElement("label");
returnCodeLabel.htmlFor = "returnCodeCheckbox";
returnCodeLabel.textContent = "Return code directly (dev/test only)";
toolbar.appendChild(returnCodeCheckbox);
toolbar.appendChild(returnCodeLabel);

const initBtn = document.createElement("button");
initBtn.textContent = "Request Approval";
initBtn.addEventListener("click", () => {
    clientId = clientIdInput.value || "webui";
    const url = `/CubeAuth-initCode?client_id=${encodeURIComponent(clientId)}${returnCodeCheckbox.checked ? "&return_code=1" : ""}`;
    fetch(url)
        .then(r => r.json())
        .then(j => {
            if (j.success) {
                if (j.initial_code) {
                    pendingCode = j.initial_code;
                    alert(`Approval requested. This code is being returned directly because dev/test mode is enabled: ${pendingCode}`);
                } else {
                    pendingCode = null;
                    alert("Approval requested on the device. Approve it there, then enter the one-time code shown on the device to finish authentication.");
                }
            } else {
                alert("Failed to request approval: " + (j.message || "unknown error"));
            }
        })
        .catch(e => alert("Error: " + e));
});
toolbar.appendChild(initBtn);

const authBtn = document.createElement("button");
authBtn.textContent = "Exchange Approved Code";
authBtn.addEventListener("click", () => {
    clientId = clientIdInput.value || "webui";
    let code = pendingCode;
    if (!code) {
        code = prompt("Enter the initial code shown on the device:");
        if (!code) return;
    }
    fetch(`/CubeAuth-authHeader?client_id=${encodeURIComponent(clientId)}&initial_code=${encodeURIComponent(code)}`)
        .then(r => r.json())
        .then(j => {
            if (j.success && j.auth_code) {
                authToken = j.auth_code;
                setCookie("cube_auth", authToken, 365);
                pendingCode = null;
                alert("Authenticated. Private endpoints will include Authorization automatically.");
            } else {
                const reason = j.error ? `${j.error}: ` : "";
                alert("Auth failed: " + reason + (j.message || "unknown error"));
            }
        })
        .catch(e => alert("Error: " + e));
});
toolbar.appendChild(authBtn);
// load the json from /getEndpoints and use that to create links on the page
fetch("/getEndpoints")
    .then((response) => response.json())
    .then((data) => {
        console.log(data);
        const endpoints = document.getElementById("endpoints");
        // iterate through the data and create a link for each endpoint
        // data = { "Endpoint_category1": [{name:"endpoint1", params:[], public: true}, {name:"endpoint2",params:["parma1","param2"], public:false}], "Endpoint_category2": [{name:"endpoint1", params:[], public: true}, {name:"endpoint2",params:["parma1","param2"], public:false}] }
        // this turns into a list of links like: Endpoint_category1-endpoint1, Endpoint_category1-endpoint2, Endpoint_category2-endpoint1, Endpoint_category2-endpoint2
        for (const category in data) {
            const categoryHeader = document.createElement("h2");
            categoryHeader.innerText = category;
            endpoints.appendChild(categoryHeader);
            for (const endpoint of data[category]) {
                // if an endpoint has params, we should have a way for the user to input them
                const endpointForm = document.createElement("form");
                endpointForm.action = `/${category}-${endpoint.name}`;
                endpointForm.method = endpoint.endpoint_type;
                console.log(endpoint);
                const params = extractParams(endpoint.params);
                for (const param of params) {
                    const input = document.createElement("input");
                    const paramType = describeParamType(param.schema);
                    input.type = paramType === "integer" || paramType === "number" ? "number" : "text";
                    input.name = param.name;
                    input.placeholder = `${param.name}${param.required ? " (required)" : ""} [${paramType}]`;
                    input.required = param.required;
                    endpointForm.appendChild(input);
                }
                const submit = document.createElement("input");
                submit.type = "submit";
                submit.value = "Submit";
                submit.addEventListener("click", (event) => {
                    event.preventDefault();
                    const headers = {};
                    if (endpoint.public !== "true" && authToken) {
                        headers["Authorization"] = `Bearer ${authToken}`;
                    }
                    if (endpoint.endpoint_type == "GET") {
                        const url = endpointForm.action + "?" + new URLSearchParams(new FormData(endpointForm));
                        fetch(url, { method: "GET", headers })
                            .then((response) => response.json().catch(() => ({ status: response.status })))
                            .then((data) => { console.log(data); alert(JSON.stringify(data)); })
                            .catch((e) => alert("Error: " + e));
                    } else {
                        const body = JSON.stringify(Object.fromEntries(new FormData(endpointForm)));
                        headers["Content-Type"] = "application/json";
                        fetch(endpointForm.action, { method: endpoint.endpoint_type, headers, body })
                            .then((response) => response.json())
                            .then((data) => { console.log(data); alert(JSON.stringify(data)); })
                            .catch((e) => alert("Error: " + e));
                    }
                });
                endpointForm.appendChild(submit);
                endpoints.appendChild(endpointForm);
                // create a link to the endpoint
                const endpointLink = document.createElement("a");
                endpointLink.href = `/${category}-${endpoint.name}`;
                const paramNames = params.map((param) => param.name).join(", ") || "none";
                endpointLink.innerText = `${category}-${endpoint.name}, params: ${paramNames}, public: ${endpoint.public}`;
                endpoints.appendChild(endpointLink);
                endpoints.appendChild(document.createElement("br"));
            }
        }
    })
    .catch((error) => {
        console.error("Failed to load endpoints", error);
        renderEndpointLoadError(`Failed to load endpoints: ${error}`);
    });
