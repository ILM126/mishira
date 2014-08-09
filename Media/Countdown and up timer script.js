//--------------------------------------------------------
// Countdown/up timer script

// Timer configuration
var target = Date.parse("1 May 2014 12:30:00 PM"); // Time to count down/up to
var countUp = true; // `true` = count up timer, `false = count down timer

function updateTimer()
{
    // Calculate the difference in time between now and the target in seconds
    var now = new Date();
    var diff;
    if(countUp)
        diff = Math.max((now - target) / 1000, 0);
    else
        diff = Math.max((target - now) / 1000, 0);

    // Convert seconds to days, hours, minutes and seconds
    var days = Math.floor(diff / 60 / 60 / 24);
    var hrs  = Math.floor(diff / 60 / 60 - days * 24);
    var mins = Math.floor(diff / 60 - (days * 24 + hrs) * 60);
    var secs = Math.floor(diff - ((days * 24 + hrs) * 60 + mins) * 60);
    
    // Display the counter
    updateText(days + "d " + hrs + "h " + mins + "m " + secs + "s");
}

// Update the text immediately and once every second
updateTimer();
setInterval(updateTimer, 1000); // 1000 msec
