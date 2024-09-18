let test = "nothing";
console.log("auth.js is loaded");
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
                endpointForm.method = "POST";
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
                    fetch(endpointForm.action, {
                        method: "POST",
                        // body is json of the form {param1: value1, param2: value2}
                        body: JSON.stringify(Object.fromEntries(new FormData(endpointForm))),
                        // must have header to tell the server that the body is json
                        headers: {
                            "Content-Type": "application/json",
                        }
                    })
                        .then((response) => response.json())
                        .then((data) => {
                            console.log(data);
                        });
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
