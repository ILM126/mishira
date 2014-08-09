//--------------------------------------------------------
// Retrieve file contents script

// Filename of the file to read
var filename = "C:/Example file.txt";

// Temporary display some static text while we are processing
var prevContents = "Please wait...";
updateText(prevContents);

function processFile()
{
    // Read the contents of the file
    var contents = readFile(filename);

    // Reduce CPU usage by only updating the text when it changes
    if(contents != prevContents) {
        // Display the file contents
        updateText(contents);

        // Remember the file contents for next time
        prevContents = contents;
    }
}

// Read the file immediately and once every two seconds
processFile();
setInterval(processFile, 2000);
