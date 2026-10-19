// Interactive JavaScript Demo Application
console.log("Interactive script loaded and running!");

let count = 0;
let counterElem = document.getElementById("counter");
let btn = document.getElementById("click-btn");
let statusElem = document.getElementById("status-text");

if (btn) {
    btn.addEventListener("click", function() {
        count = count + 1;
        if (counterElem) {
            counterElem.setTextContent("Clicks: " + count);
        }
        if (statusElem) {
            statusElem.setTextContent("Button was clicked! Total count is now " + count);
        }
        console.log("Button clicked! Count:", count);
    });
}

