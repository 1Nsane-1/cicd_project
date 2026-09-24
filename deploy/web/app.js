function triggerApi(endpoint) {
  fetch(endpoint, { method: "POST" });
}

function updateLogs() {
  fetch("/api/logs/run_1")
    .then((r) => r.json())
    .then((d) => {
      const el = document.getElementById("logs");
      el.textContent = d.logs;
      el.scrollTop = el.scrollHeight;
    });
}

function updateStatus() {
  fetch("/api/status/run_1")
    .then((r) => r.json())
    .then((d) => {
      document.getElementById("status").textContent = d.status;
    });
}

setInterval(() => {
  updateLogs();
  updateStatus();
}, 1000);

const evtSource = new EventSource("/api/events");
evtSource.addEventListener("status", (e) => {
  document.getElementById("status").textContent = e.data;
});
