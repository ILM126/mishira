//--------------------------------------------------------
// Retrieve website title script

// URL of the website to read
var url = "http://www.mishira.com";

// Temporary display some static text while we are processing
updateText("Please wait...");

// Fetch the HTML of the specified website
var html = readHttpGet(url);

// Find the `<title>` tag using regular expression
var matches = html.match(/<title>(.*)<\/title>/);
var title;
if(matches != null)
    title = matches[1];
else
    title = "No <title> found";

// Display the title
updateText(title);
