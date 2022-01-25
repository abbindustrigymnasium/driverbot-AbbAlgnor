// called when the client connects
let connected = false;
let servoAdjustment = 0;

function onConnect() {
    connected = true;
    // Once a connection has been made, make a subscription and send a message.
    let topic = document.forms["connectionForm"]["topic"].value
    console.log("onConnect");
    log("Connected!", "LogTopic")
    client.subscribe("DriverBotLogTopic");
    client.subscribe("DriverBotStatus");
    message = new Paho.MQTT.Message("Hello from web");
    message.destinationName = topic;
    client.send(message);

    startGamepadInput();
    startKeyInputs();
}

// called when the client loses its connection
function onConnectionLost(responseObject) {
    if (responseObject.errorCode !== 0) {
        console.log("onConnectionLost:" + responseObject.errorMessage);
    }
    log("Connnnection lost: <span class='text'>ErrorCode: </span>" + responseObject.errorCode, "LogTopic", 2)
}

// called when a message arrives
function onMessageArrived(message) {
  console.log(message.destinationName)
    if (message.destinationName == "DriverBotLogTopic") {
    log("<span class='topic'>" + message.destinationName + "</span>: " + message.payloadString, message.destinationName)
    }
    else if (message.destinationName == "DriverBotStatus") {
       document.getElementById("DriverBotStatus").innerHTML = syntaxHighlight(JSON.stringify(message.payloadString, undefined, 2)) 
    }
}

function send(sendMessage) {
    let topic = document.forms["connectionForm"]["topic"].value


    message = new Paho.MQTT.Message(JSON.stringify(sendMessage));
    message.destinationName = topic;
    client.send(message);

    document.getElementById("lastMessage").innerHTML = syntaxHighlight(JSON.stringify(sendMessage, undefined, 2))


}

var coll = document.getElementsByClassName("collapsible");
var i;

for (i = 0; i < coll.length; i++) {
    coll[i].addEventListener("click", function () {
        this.classList.toggle("active");
        var content = this.nextElementSibling;
        if (content.style.maxHeight) {
            content.style.maxHeight = null;
        } else {
            content.style.maxHeight = content.scrollHeight + "px";
        }
    });
}

function connect() {
    let input = document.forms["connectionForm"]

    let ip = input["ip"].value
    let port = parseInt(input["port"].value)
    let topic = input["topic"].value;

    log(`Connecting to:<span class="text"> <a href="${"ws://" + ip + ":" + port + "/" + topic}">${"ws://" + ip + ":" + port + "/" + topic}</a></span>`, "LogTopic")
    // Create a client instance
    client = new Paho.MQTT.Client(ip, port, "/" + topic, "WebClient-LetsHopeOnlyOneConnectionExists");

    // set callback handlers
    client.onConnectionLost = onConnectionLost;
    client.onMessageArrived = onMessageArrived;

    // connect the client
    client.connect({ onSuccess: (onConnect) });
}

function updateInputType() {
    let button = document.getElementById("toggle")
    console.log(button.checked)

    if (button.checked) {
        document.getElementById("input").innerHTML = "";
    } else {
        document.getElementById("input").innerHTML = "";
    }
}

function syntaxHighlight(json) {
    json = json.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
    return json.replace(/("(\\u[a-zA-Z0-9]{4}|\\[^u]|[^\\"])*"(\s*:)?|\b(true|false|null)\b|-?\d+(?:\.\d*)?(?:[eE][+\-]?\d+)?)/g, function (match) {
        var cls = 'number';
        if (/^"/.test(match)) {
            if (/:$/.test(match)) {
                cls = 'key';
            } else {
                cls = 'string';
            }
        } else if (/true|false/.test(match)) {
            cls = 'boolean';
        } else if (/null/.test(match)) {
            cls = 'null';
        }
        return '<span class="' + cls + '">' + match + '</span>';
    });
}

let pageConsole

function getTimestamp() {
    let time = new Date(Date.now());
    return `${String(time.getHours()).padStart(2, '0')}:${String(time.getMinutes()).padStart(2, '0')}:${String(time.getSeconds()).padStart(2, '0')}`;
}

function log(message, target, type = "append", severity = 0) {
    const severityTags = ["INFO", "WARN", "ERR"];

    format = `<span class="${severityTags[severity]}"><span class="text">[${getTimestamp()}</span> ${severityTags[severity]}<span class="text">]</span> ${message}</span><br/>`;

    if (pageConsole == null)
        pageConsole = document.getElementById(target);
    if (type == "append") {
    pageConsole.innerHTML += format;
    }
    if (type == "set") {
    pageConsole.innerHTML = format;
    }
}

function adjustServo(adjustment) {
    servoAdjustment += adjustment
    if (connected) {
        send({Servo: {servoOffset: servoAdjustment}});
    }
}
function pidTune(p, i, d) {
    data = {}
    data.p = parseFloat(document.forms["PID"]["p"].value)
    data.i = parseFloat(document.forms["PID"]["i"].value)
    data.d = parseFloat(document.forms["PID"]["d"].value)
    console.log(data, )
    send({pid: data})
}
