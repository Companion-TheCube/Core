let test = "nothing";
let authToken = null;
let clientId = "webui"; // default; user can override below
let pendingCode = null;

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

const confirmCheckbox = document.createElement("input");
confirmCheckbox.type = "checkbox";
confirmCheckbox.id = "confirmCheckbox";
const confirmLabel = document.createElement("label");
confirmLabel.htmlFor = "confirmCheckbox";
confirmLabel.textContent = "Use device confirmation";
toolbar.appendChild(confirmCheckbox);
toolbar.appendChild(confirmLabel);

const initBtn = document.createElement("button");
initBtn.textContent = "Get Initial Code";
initBtn.addEventListener("click", () => {
    clientId = clientIdInput.value || "webui";
    const url = `/CubeAuth-initCode?client_id=${encodeURIComponent(clientId)}${confirmCheckbox.checked ? "&return_code=1" : ""}`;
    fetch(url)
        .then(r => r.json())
        .then(j => {
            if (j.success) {
                if (j.initial_code) {
                    pendingCode = j.initial_code;
                    alert(`Confirm this code on the device: ${pendingCode}`);
                } else {
                    alert("Initial code generated on the device. Enter that code here to finish authentication.");
                }
            } else {
                alert("Failed to get initial code: " + (j.message || "unknown error"));
            }
        })
        .catch(e => alert("Error: " + e));
});
toolbar.appendChild(initBtn);

const authBtn = document.createElement("button");
authBtn.textContent = "Authenticate";
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
                alert("Auth failed: " + (j.message || "unknown error"));
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
                for (const param of endpoint.params) {
                    const input = document.createElement("input");
                    input.type = "text";
                    input.name = param;
                    input.placeholder = param;
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
                endpointLink.innerText = `${category}-${endpoint.name}, params: ${endpoint.params}, public: ${endpoint.public}`;
                endpoints.appendChild(endpointLink);
                endpoints.appendChild(document.createElement("br"));
            }
        }
    });
