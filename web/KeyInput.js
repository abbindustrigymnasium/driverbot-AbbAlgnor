function startKeyInputs() {
    document.addEventListener('keydown', keyDown);
    document.addEventListener('keyup', keyUp);
}

let key = {
    w: 0,
    a: 0,
    s: 0,
    d: 0
}

let lastKeyMessage = {};

let keyMessage = {
    Forward: 0,
    Backward: 0,
    Turning: 0
}

function keyUp(keypress) {
    switch (keypress.code) {
        case "KeyW":
            key.w = 0;
            break;
        case "KeyA":
            key.a = 0;
            break
        case "KeyS":
            key.s = 0;
            break
        case "KeyD":
            key.d = 0;
            break
    }
    sendMessage()
}
function keyDown(keypress) {
    switch (keypress.code) {
        case "KeyW":
            key.w = 1;
            break;
        case "KeyA":
            key.a = 1;
            break
        case "KeyS":
            key.s = 1;
            break
        case "KeyD":
            key.d = 1;
            break
    }
    sendMessage()
}

function sendMessage() {
    let speed = document.getElementById("speed").value
    keyMessage.Forward = key.w * speed
    keyMessage.Backward = key.s * speed
    keyMessage.Turning = key.a * 45 - key.d * 45

    if (JSON.stringify(keyMessage) !== JSON.stringify(lastKeyMessage)) {

        send({drive: keyMessage})
        message = keyMessage

        lastKeyMessage = JSON.parse(JSON.stringify(keyMessage));
        //lastKeyMessage = keyMessage
    }
}
