var main = (function (executor) {

    var SENSOR_POLL = 300;
    var MOTOR_POLL = 20;
    var joystick;
    var pingStart;
    var sensorStart;
    var motorStart;
    var moving;

    //Modify By Eric Chen
    var joystickL;
    var joystickR;
    var s = function (sel) { return document.querySelector(sel); };
    var sId = function (sel) { return document.getElementById(sel); };
    var removeClass = function (el, clss) {
        el.className = el.className.replace(new RegExp('\\b' + clss + ' ?\\b', 'g'), '');
    }

    // Get debug elements and map them
    var elDebug = sId('debug');
    var elDump = elDebug.querySelector('.dump');
    var els = {
        position: {
            x: elDebug.querySelector('.position .x .data'),
            y: elDebug.querySelector('.position .y .data')
        },
        force: elDebug.querySelector('.force .data'),
        pressure: elDebug.querySelector('.pressure .data'),
        distance: elDebug.querySelector('.distance .data'),
        angle: {
            radian: elDebug.querySelector('.angle .radian .data'),
            degree: elDebug.querySelector('.angle .degree .data')
        },
        direction: {
            x: elDebug.querySelector('.direction .x .data'),
            y: elDebug.querySelector('.direction .y .data'),
            angle: elDebug.querySelector('.direction .angle .data')
        }
    };

    var timeoutCreate;
    var nbEvents = 0;

    function createThrottle (evt) {
        clearTimeout(timeoutCreate);
        timeoutCreate = setTimeout(() => {
            activatePanel(evt);
        }, 100);
    }


    //綁定搖桿
    //命令代碼code
    //1:左搖桿事件
    //2:右搖桿事件
    //命令事件
    //1:start
    //2:end
    //3:move
    //方向
    //1:up
    //2:down
    //3:right
    //4:left
    function bindNipple () {
        joystickL.on('start', function (evt, data) {
            var cmd = new Object();
            cmd.code = 1;
            cmd.evt = 1;
            executor.send(JSON.stringify(cmd));
            // dump(evt.type);
            // debug(data);
            // executor.send(JSON.stringify(data));
        }).on('end', function (evt, data) {
            var cmd = new Object();
            cmd.code = 1;
            cmd.evt = 2;
            executor.send(JSON.stringify(cmd));
            // debug(data);
            // executor.send(JSON.stringify(data));
        }).on('move', function (evt, data) {
            var cmd = new Object();
            // debug(data);
            cmd.code = 1;
            cmd.evt = 3;
            //左搖桿只要關心上下方向跟移動距離
            cmd.direction = (data.direction.y == 'up')?1:2;
            //distance只要整數
            cmd.distance = Math.floor(data.distance);
            executor.send(JSON.stringify(cmd));
        }).on('dir:up plain:up dir:left plain:left dir:down ' +
            'plain:down dir:right plain:right',
            function (evt, data) {
                //所在區塊這邊不處理
                // dump(evt.type);
                // executor.send(JSON.stringify(data));
            }
        ).on('pressure', function (evt, data) {
            //按壓力道也不處理
            // debug({pressure: data});
            // executor.send(data);
        });
        //綁定右搖桿
        joystickR.on('start', function (evt, data) {
            var cmd = new Object();
            cmd.code = 2;
            cmd.evt = 1;
            executor.send(JSON.stringify(cmd));
            // executor.send(JSON.stringify(data));
        }).on('end', function (evt, data) {
            var cmd = new Object();
            cmd.code = 2;
            cmd.evt = 2;
            executor.send(JSON.stringify(cmd));
            // debug(data);
            // executor.send(JSON.stringify(data));
        }).on('move', function (evt, data) {
            var cmd = new Object();
            // debug(data);
            cmd.code = 2;
            cmd.evt = 3;
            //右搖桿只要關心左右方向跟移動距離
            cmd.direction = (data.direction.x == 'right')?3:4;
            //distance只要整數
            cmd.distance = Math.floor(data.distance);
            executor.send(JSON.stringify(cmd));
        }).on('dir:up plain:up dir:left plain:left dir:down ' +
            'plain:down dir:right plain:right',
            function (evt, data) {
                //所在區塊這邊不處理
                // dump(evt.type);
                // executor.send(JSON.stringify(data));
            }
        ).on('pressure', function (evt, data) {
            //按壓力道也不處理
            // debug({pressure: data});
            // executor.send(data);
        });
    }

    function activatePanel (evt) {
        var type = typeof evt === 'string' ?
            evt : evt.target.getAttribute('data-type');

        removeClass(s('.zone.active'), 'active');
        removeClass(s('.button.active'), 'active');
        s('.button.' + type).className += ' active';
        s('.zone.' + type).className += ' active';
    }

    // Print data into elements
    function debug (obj) {
        function parseObj(sub, el) {
            for (var i in sub) {
                if (typeof sub[i] === 'object' && el) {
                    parseObj(sub[i], el[i]);
                } else if (el && el[i]) {
                    el[i].innerHTML = sub[i];
                }
            }
        }
        setTimeout(function () {
            parseObj(obj, els);
        }, 0);
    }


    // Dump data
    function dump (evt) {
        setTimeout(function () {
            if (elDump.children.length > 4) {
                elDump.removeChild(elDump.firstChild);
            }
            var newEvent = document.createElement('div');
            newEvent.innerHTML = '#' + nbEvents + ' : <span class="data">' +
                evt + '</span>';
            elDump.appendChild(newEvent);
            nbEvents += 1;
        }, 0);
    }

    function sendPing() {
        $("#pingTime").text('');
        pingStart = new Date().getTime();
        var ba = new Uint8Array(1);
        ba[0] = 0x0A;
        executor.send(ba);
    }

    var pollSensors = function () {
        sendSensor();
    };

    var pollMotor = function () {
        var coord = calc();
        var cmd = buildCmd(coord);
        $("#result").text(cmd);
        motorStart = new Date().getTime();
        executor.send(cmd.buffer);
    };

    function onReplyMotor() {
        var endTime = new Date().getTime();
        var latency = endTime - motorStart;
        //updateLatency(latency);
        var nextRun = MOTOR_POLL - latency;
        if (nextRun < 0) {
            nextRun = 0;
        }
        if (!moving) {
            return;
        }
        // TODO clear
        setTimeout(pollMotor, nextRun);

        $("#motorLatency").text(latency);
    }

    function sendSensor() {
        sensorStart = new Date().getTime();
        var ba = new Uint8Array(1);
        ba[0] = 0x0B;
        executor.send(ba);
    }

    function displaySensor(bytearray) {
        var uptime = (bytearray[1] << 16) | (bytearray[2] << 8) | bytearray[3];
        var amp = (bytearray[4] << 8) | bytearray[5];
        var volt = (bytearray[6] << 8) | bytearray[7];

        $("#uptime").text(uptime);

        var AMP_OFFSET = 2500;
        var BATT_AMP_VOLTS_PER_AMP = 66;
        var aaa = ((((amp / 1024.0) * 5000) - AMP_OFFSET) / BATT_AMP_VOLTS_PER_AMP);

        $("#amp").text(aaa);
        $("#volt").text((47 + 10) * volt * 5 / (1024 * 10));

        var endTime = new Date().getTime();
        var latency = endTime - sensorStart;
        //updateLatency(latency);
        var nextRun = SENSOR_POLL - latency;
        if (nextRun < 0) {
            nextRun = 0;
        }
        setTimeout(pollSensors, nextRun);

        $("#sensorLatency").text(latency);
    }

    function displayPong() {
        var end = new Date().getTime();
        var time = end - pingStart;
        $("#pingTime").text(time);
    }

    function runF(num) {
        var ba = buildCmd1Wheel(num, 1, 255);
        executor.send(ba);
    }

    function runB(num) {
        var ba = buildCmd1Wheel(num, 2, 255);
        executor.send(ba);
    }

    function stopWheel(num) {
        var ba = buildCmd1Wheel(num, 3, 255);
        executor.send(ba);
    }

    var sc45 = sinDegrees(45);
    var R1 = 100;
    var R2 = R1 * sc45;

    function buildCmd(coord) {
        var x = coord.x;
        var y = coord.y;
        if (y > R2 && Math.abs(x) <= y) {
            x = R2 * x / y;
            y = R2;
        } else if (x > R2 && Math.abs(y) <= x) {
            y = R2 * y / x;
            x = R2;
        } else if (y < -R2 && Math.abs(x) <= -y) {
            x = -R2 * x / y;
            y = -R2;
        } else if (x < -R2 && Math.abs(y) <= -x) {
            y = -R2 * y / x;
            x = -R2;
        }

        x = x * 255 / R2;
        y = y * 255 / R2;

        //var l = getDirAndSpeed(Math.round(x));
        //var r = getDirAndSpeed(Math.round(y));
        //return '#' + l.d + l.s + r.d + r.s;
        var bytearray = buildCmdAllWheels(x, y);
        return bytearray;
    }

    function buildCmdAllWheels(x, y) {
        var bytearray = new Uint8Array(5);
        bytearray[0] = 0x0C;
        bytearray[1] = getDir(x);
        var s1 = getSpeed(x);
        s1 = incr(s1);
        bytearray[2] = s1;
        bytearray[3] = getDir(y);
        var s2 = getSpeed(y);
        s2 = incr(s2);
        bytearray[4] = s2;
        return bytearray;
    }

    function incr(a) {
        // (x-R)2 + y2 = R2
        return Math.round(Math.sqrt(2*a*255 - a*a));
    }

    function buildCmd1Wheel(wheel, dir, speed) {
        var ba = new Uint8Array(4);
        ba[0] = 0x0D;
        ba[1] = wheel;
        ba[2] = dir;
        ba[3] = speed;
        return ba;
    }

    function getDir(c) {
        return c >= 0 ? 1 : 2;
    }

    function getSpeed(c) {
        if (c < 0) {
            c = -c;
        }
        return c;
    }

    function pad(num) {
        var s = "00" + num;
        return s.substr(s.length - 3);
    }

    function calc() {
        var x = joystick.deltaX();
        var y = -joystick.deltaY();
        var x1 = sc45 * (x + y);
        var y1 = sc45 * (-x + y);
        return {x: x1, y: y1};
    }

    function sinDegrees(angle) {
        return Math.sin(angle / 180 * Math.PI);
    }

    function initJoystick() {
        joystickL = nipplejs.create({
            zone: sId('leftJoystick'),
            mode: 'static',
            position: { left: '20%', top: '50%' },
            color: 'green',
            size: 200
        });

        joystickR = nipplejs.create({
            zone: sId('rightJoystick'),
            mode: 'static',
            position: { left: '80%', top: '50%' },
            color: 'red',
            size: 200
        });

        bindNipple();
    }

    function initWheels() {
        $(".wheelControlF").click(function () {
            var wnum = $(this).parent(".wheelControl").data("wnum");
            runF(parseInt(wnum));
        });
        $(".wheelControlB").click(function () {
            var wnum = $(this).parent(".wheelControl").data("wnum");
            runB(parseInt(wnum));
        });
        $(".wheelControlS").click(function () {
            var wnum = $(this).parent(".wheelControl").data("wnum");
            stopWheel(parseInt(wnum));
        });
    }

    function onMsg(event) {
        sId('pingTime').value = event.data;
        // var bytearray = new Uint8Array(event.data);
        // if (bytearray[0] == 0x0A) {
        //     displayPong();
        // } else if (bytearray[0] == 0x0B && bytearray.length == 8) {
        //     displaySensor(bytearray)
        // } else if (bytearray[0] == 0x0C) {
        //     onReplyMotor();
        // }
    }

    function buildCmd2Servo(servo1, servo2) {
        var ba = new Uint8Array(3);
        ba[0] = 0x10;
        ba[1] = servo1;
        ba[2] = servo2;
        return ba;
    }

    function servoCenter() {
        var ba = buildCmd2Servo(90, 90);
        executor.send(ba);
        $("#servo1range").val(90);
        $("#servo2range").val(90);
    }

    function moveServoH(val) {
        console.log("H: " + val);
        var ba = new Uint8Array(2);
        ba[0] = 0x11;
        ba[1] = val;
        executor.send(ba);
    }

    function moveServoV(val) {
        console.log("V: " + val);
        var ba = new Uint8Array(2);
        ba[0] = 0x12;
        ba[1] = val;
        executor.send(ba);
    }

    function initServo() {
        $("#cameraCenterBtn").click(function () {
            servoCenter();
        });
        // TODO
        $('input[type=range]').on('input', function () {
            $(this).trigger('change');
        });
        $("#servo1range").change(function () {
            var newval=$(this).val();
            moveServoH(newval);
        });
        $("#servo2range").change(function () {
            var newval=$(this).val();
            moveServoV(newval);
        });
    }

    var imageNr = 0; // Serial number of current image
    var finished = new Array(); // References to img objects which have finished downloading
    var paused = false;

    function createImageLayer() {
        var img = new Image();
        img.style.position = "absolute";
        img.style.zIndex = -1;
        img.onload = imageOnload;
        img.onclick = imageOnclick;
        img.src = "http://192.168.2.50:8080/?action=snapshot&n=" + (++imageNr);
        img.width = 300;
        var webcam = document.getElementById("webcam");
        webcam.insertBefore(img, webcam.firstChild);
    }

// Two layers are always present (except at the very beginning), to avoid flicker
    function imageOnload() {
        this.style.zIndex = imageNr; // Image finished, bring to front!
        while (1 < finished.length) {
            var del = finished.shift(); // Delete old image(s) from document
            del.parentNode.removeChild(del);
        }
        finished.push(this);
        if (!paused) createImageLayer();
    }

    function imageOnclick() { // Clicking on the image will pause the stream
        paused = !paused;
        if (!paused) createImageLayer();
    }

    function testWebSocket() {
        websocket = new WebSocket('ws://192.168.4.1/ws');
        websocket.onopen = function (evt) {
          onOpen(evt)
        };
     
        websocket.onclose = function (evt) {
          onClose(evt)
        };
     
        websocket.onmessage = function (evt) {
          onMessage(evt)
        };
     
        websocket.onerror = function (evt) {
          onError(evt)
        };
      }
     
      function onOpen(evt) {
        writeToScreen("CONNECTED");
        doSend("WebSocket rocks");
      }
     
      function onClose(evt) {
        writeToScreen("DISCONNECTED");
      }
     
      function onMessage(evt) {
        writeToScreen('<span style="color:blue;">RESPONSE:'+evt.data+'</span>');
        //websocket.close();
      }
     
      function onError(evt) {
        writeToScreen('<span style="color:red;">ERROR:</span>' + evt.data);
      }
     
      function doSend(message) {
        writeToScreen("SENT: " + message);
        websocket.send(message);
      }
     
      function writeToScreen(message) {
        $("#errorMsg").text(message);
        $("#errorMsg").show();
      }
 
      function docReady() {
        console.log("ready!");
        executor.init(onMsg);
        //executor.startJob(pollSensors);
        sId('buttons').onclick = sId('buttons').ontouchend = createThrottle;
        sId('btnDisconnect').onclick = function(){
            executor.send('Hello');
        }
        activatePanel('joystick');

        initJoystick();
        //initWheels();
        //initServo();
        // $("#pingBtn").click(function () {
        //     sendPing();
        // });
        // createImageLayer();
        //sendPing();
        //sendSensor();
        //testWebSocket();
    }

    var init = function () {
        $(document).ready(docReady);
    };

    return {
        init: init
    };

})(executor);
