
// https://stackoverflow.com/a/27220617

function findserver(port, ipBase, ipLow, ipHigh, maxInFlight, timeout, cb) {
    var ipCurrent = +ipLow, numInFlight = 0, found = false;
    ipHigh = +ipHigh;

    function tryOne(ip) {
        ++numInFlight;
        var address = "ws://" + ipBase + ip + ":" + port;
        var socket = new WebSocket(address);
        var timer = setTimeout(function() {
            console.log(address + " timeout");
            var s = socket;
            socket = null;
            s.close();
            --numInFlight;
            next();
        }, timeout);
        socket.onopen = function() {
            if (socket) {
                found = true;
                console.log(address + " success");
                //clearTimeout(timer);
                cb(socket.url);
            }
        };
        socket.onerror = function(err) {
            if (socket) {
                console.log(address + " error");
                clearTimeout(timer);
                --numInFlight;
                next();
            }
        }
    }

    function next() {
        while (ipCurrent <= ipHigh && numInFlight < maxInFlight && !found) {
            tryOne(ipCurrent++);
        }
        // if we get here and there are no requests in flight, then
        // we must be done
        if (numInFlight === 0) {
            if (!found) {
                console.log("ws not found, giving up");
                cb(null);
            }
        }
    }

    next();
}

/*
findServers(1234, "192.168.1.", 1, 255, 20, 1, 4000, function(servers) {
    console.log(servers);
});
*/
