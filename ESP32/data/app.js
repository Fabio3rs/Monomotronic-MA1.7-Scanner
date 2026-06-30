(function () {
  const storageKeys = {
    tab: "ma17.activeTab",
    technicalMode: "ma17.technicalMode",
    lastStatus: "ma17.lastStatus",
    lastSensors: "ma17.lastSensors",
    lastErrors: "ma17.lastErrors",
    lastHealth: "ma17.lastHealth",
    lastTechnical: "ma17.lastTechnical"
  };

  const state = {
    activeTab: localStorage.getItem(storageKeys.tab) || "live",
    technicalMode: localStorage.getItem(storageKeys.technicalMode) === "true",
    requestInFlight: false,
    status: readJson(storageKeys.lastStatus),
    sensors: readJson(storageKeys.lastSensors),
    errors: readJson(storageKeys.lastErrors),
    health: readJson(storageKeys.lastHealth),
    technical: readJson(storageKeys.lastTechnical),
    pollTimer: null
  };

  const els = {
    liveBadge: byId("liveBadge"),
    cacheBadge: byId("cacheBadge"),
    pollInfo: byId("pollInfo"),
    messageBox: byId("messageBox"),
    statusGrid: byId("statusGrid"),
    stateSummary: byId("stateSummary"),
    cacheSummary: byId("cacheSummary"),
    initStrings: byId("initStrings"),
    sensorList: byId("sensorList"),
    errorList: byId("errorList"),
    healthList: byId("healthList"),
    technicalList: byId("technicalList"),
    connectBtn: byId("connectBtn"),
    refreshBtn: byId("refreshBtn"),
    technicalToggleBtn: byId("technicalToggleBtn"),
    loadSensorsBtn: byId("loadSensorsBtn"),
    readErrorsBtn: byId("readErrorsBtn"),
    clearErrorsBtn: byId("clearErrorsBtn"),
    loadHealthBtn: byId("loadHealthBtn"),
    readMemoryBtn: byId("readMemoryBtn"),
    loadCatalogBtn: byId("loadCatalogBtn"),
    tableSelect: byId("tableSelect"),
    memHi: byId("memHi"),
    memLo: byId("memLo"),
    memLen: byId("memLen")
  };

  bindEvents();
  renderAll();
  pollStatus(true);

  function byId(id) {
    return document.getElementById(id);
  }

  function readJson(key) {
    try {
      const raw = localStorage.getItem(key);
      return raw ? JSON.parse(raw) : null;
    } catch (_) {
      return null;
    }
  }

  function writeJson(key, value) {
    localStorage.setItem(key, JSON.stringify(value));
  }

  function bindEvents() {
    document.querySelectorAll(".tab").forEach((button) => {
      button.addEventListener("click", () => setTab(button.dataset.tab));
    });

    els.connectBtn.addEventListener("click", () =>
      runAction("/api/connect", { method: "POST" }, onStatusResponse));
    els.refreshBtn.addEventListener("click", () => pollStatus(true));
    els.technicalToggleBtn.addEventListener("click", () => {
      state.technicalMode = !state.technicalMode;
      localStorage.setItem(storageKeys.technicalMode, String(state.technicalMode));
      renderAll();
    });
    els.loadSensorsBtn.addEventListener("click", loadSensors);
    els.readErrorsBtn.addEventListener("click", loadErrors);
    els.clearErrorsBtn.addEventListener("click", clearErrors);
    els.loadHealthBtn.addEventListener("click", loadHealth);
    els.readMemoryBtn.addEventListener("click", loadMemory);
    els.loadCatalogBtn.addEventListener("click", loadCatalog);
  }

  function setTab(tab) {
    state.activeTab = tab;
    localStorage.setItem(storageKeys.tab, tab);
    renderTabs();
    schedulePolling();
  }

  function renderTabs() {
    document.querySelectorAll(".tab").forEach((button) => {
      button.classList.toggle("active", button.dataset.tab === state.activeTab);
    });

    ["live", "dtc", "health", "technical"].forEach((name) => {
      byId(`tab-${name}`).classList.toggle("section-hidden", state.activeTab !== name);
    });
  }

  function renderAll() {
    renderTabs();
    renderStatus();
    renderCacheSummary();
    renderTechnicalMode();
    renderCachedSections();
    syncControls();
  }

  function renderStatus() {
    const status = state.status;
    if (!status) {
      els.statusGrid.innerHTML = "";
      els.stateSummary.textContent = "No status response yet.";
      return;
    }

    const metrics = [
      ["Protocol", status.protocol_state || "unknown"],
      ["Operation", status.current_operation || "none"],
      ["Connected", status.connected ? "yes" : "no"],
      ["Ready", status.init_ready ? "yes" : "no"],
      ["Busy", status.busy ? "yes" : "no"],
      ["Error code", String(status.error_code ?? 0)]
    ];

    els.statusGrid.innerHTML = metrics.map(([label, value]) => `
      <div class="metric">
        <div class="metric-label">${escapeHtml(label)}</div>
        <div class="metric-value">${escapeHtml(value)}</div>
      </div>
    `).join("");

    const initStrings = Array.isArray(status.init_strings) ? status.init_strings : [];
    els.initStrings.innerHTML = initStrings.length
      ? initStrings.map((text) => `<div class="list-item mono">${escapeHtml(text)}</div>`).join("")
      : '<div class="list-item small">Init strings become available only after ECU ready state.</div>';

    els.stateSummary.className = `message ${messageTone(status.protocol_state)}`;
    els.stateSummary.textContent =
      `state=${status.protocol_state} busy=${status.busy} ready=${status.init_ready} ` +
      `op=${status.current_operation} err=${status.error_code}`;

    els.liveBadge.textContent = status.connected
      ? `ECU ${status.protocol_state.toUpperCase()}`
      : `ECU ${status.protocol_state.toUpperCase()}`;

    els.pollInfo.textContent = describePolling(status);
  }

  function renderCacheSummary() {
    const pieces = [];
    if (state.status) pieces.push(`status:${state.status.protocol_state}`);
    if (state.sensors && state.sensors.sensors) pieces.push(`sensors:${state.sensors.sensors.length}`);
    if (state.errors && state.errors.errors) pieces.push(`dtc:${state.errors.errors.length}`);
    if (state.health && state.health.health) pieces.push("health:1");
    if (state.technical) pieces.push("technical:1");

    if (!pieces.length) {
      els.cacheBadge.textContent = "Cache empty";
      els.cacheSummary.textContent = "Nothing stored locally.";
      return;
    }

    els.cacheBadge.textContent = `Cache ${pieces.join(" | ")}`;
    els.cacheSummary.textContent =
      "Stored locally: " + pieces.join(", ") +
      ". Cached values are never replayed as ECU commands.";
  }

  function renderTechnicalMode() {
    els.technicalToggleBtn.textContent = state.technicalMode
      ? "Technical mode on"
      : "Technical mode off";
    byId("tab-technical").querySelector(".small").textContent = state.technicalMode
      ? "Technical mode enabled. Raw memory and catalog are visible here."
      : "Technical mode disabled. This tab stays for explicit technical access only.";
  }

  function renderCachedSections() {
    renderSensors(state.sensors);
    renderErrors(state.errors);
    renderHealth(state.health);
    renderTechnical(state.technical);
  }

  function renderSensors(payload) {
    const sensors = payload && Array.isArray(payload.sensors) ? payload.sensors : [];
    if (!sensors.length) {
      els.sensorList.innerHTML = '<div class="list-item small">No live sensor snapshot loaded yet.</div>';
      return;
    }

    els.sensorList.innerHTML = sensors.map((sensor) => `
      <div class="table-row">
        <div>
          <strong>${escapeHtml(sensor.name || sensor.key || `Sensor ${sensor.id}`)}</strong>
          <div class="small">id=${sensor.id} sub=${sensor.subcommand} slot=${sensor.slot}</div>
        </div>
        <div class="mono">${escapeHtml(String(sensor.raw))}</div>
        <div>${sensor.value !== undefined ? escapeHtml(String(sensor.value)) : "-"} ${escapeHtml(sensor.unit || "")}</div>
      </div>
    `).join("");
  }

  function renderErrors(payload) {
    const errors = payload && Array.isArray(payload.errors) ? payload.errors : [];
    if (!errors.length) {
      els.errorList.innerHTML = '<div class="list-item small">No DTC snapshot loaded yet.</div>';
      return;
    }

    els.errorList.innerHTML = errors.map((entry) => `
      <div class="list-item">
        <div><strong class="${entry.present ? "err" : "warn"}">${entry.present ? "Present" : "Stored"}</strong></div>
        <div>${escapeHtml(entry.description || "Unknown DTC")}</div>
        <div class="small mono">frame=${entry.packet ? entry.packet.frame : "?"}</div>
      </div>
    `).join("");
  }

  function renderHealth(payload) {
    const health = payload && payload.health ? payload.health : null;
    if (!health) {
      els.healthList.innerHTML = '<div class="list-item small">No health snapshot loaded yet.</div>';
      return;
    }

    const rows = Object.entries(health).map(([key, value]) => `
      <div class="list-item row">
        <strong>${escapeHtml(key)}</strong>
        <span class="mono">${escapeHtml(String(value))}</span>
      </div>
    `).join("");
    els.healthList.innerHTML = rows;
  }

  function renderTechnical(payload) {
    if (!payload) {
      els.technicalList.innerHTML = '<div class="list-item small">No technical data loaded yet.</div>';
      return;
    }

    if (payload.sensors) {
      els.technicalList.innerHTML = `
        <div class="list-item">
          <strong>Catalog</strong>
          <div class="small">entries=${payload.sensors.length}</div>
          <div class="mono">${escapeHtml(payload.sensors.slice(0, 10).map((s) => s.key).join(", "))}</div>
        </div>
      `;
      return;
    }

    if (payload.packets) {
      els.technicalList.innerHTML = payload.packets.map((packet, index) => `
        <div class="list-item">
          <strong>Packet ${index + 1}</strong>
          <div class="mono">${escapeHtml(JSON.stringify(packet))}</div>
        </div>
      `).join("");
    }
  }

  function syncControls() {
    const status = state.status || {};
    const ready = status.protocol_state === "ready" && status.init_ready && !status.busy;
    const threadStarted = Boolean(status.thread_started);
    const technicalEnabled = state.technicalMode;

    els.connectBtn.disabled = state.requestInFlight || threadStarted;
    els.refreshBtn.disabled = state.requestInFlight;
    els.loadSensorsBtn.disabled = state.requestInFlight || !ready;
    els.readErrorsBtn.disabled = state.requestInFlight || !ready;
    els.clearErrorsBtn.disabled = state.requestInFlight || !ready;
    els.loadHealthBtn.disabled = state.requestInFlight;
    els.readMemoryBtn.disabled = state.requestInFlight || !ready || !technicalEnabled;
    els.loadCatalogBtn.disabled = state.requestInFlight || !technicalEnabled;
  }

  async function pollStatus(forceMessage) {
    await runAction("/api/status", {}, (payload) => {
      onStatusResponse(payload);
      if (forceMessage) {
        setMessage("Status refreshed from ESP32.", "ok");
      }
    }, { silentBusy: true });
    schedulePolling();
  }

  function onStatusResponse(payload) {
    state.status = payload;
    writeJson(storageKeys.lastStatus, payload);
    renderAll();
  }

  function schedulePolling() {
    clearTimeout(state.pollTimer);
    const status = state.status || {};
    if (state.requestInFlight) {
      return;
    }

    const delay = status.protocol_state === "ready" ? 2500 : 1500;
    state.pollTimer = setTimeout(async () => {
      await pollStatus(false);

      const latest = state.status || {};
      if (latest.protocol_state === "ready") {
        if (state.activeTab === "live") {
          await loadSensors(true);
        } else if (state.activeTab === "health") {
          await loadHealth(true);
        }
      }
    }, delay);
  }

  async function loadSensors(silent) {
    const table = encodeURIComponent(els.tableSelect.value);
    await runAction(`/api/sensors/collection?table=${table}`, {}, (payload) => {
      state.sensors = payload;
      writeJson(storageKeys.lastSensors, payload);
      renderSensors(payload);
      renderCacheSummary();
      if (!silent) setMessage("Live sensor snapshot updated.", "ok");
    });
  }

  async function loadErrors() {
    await runAction("/api/errors", {}, (payload) => {
      state.errors = payload;
      writeJson(storageKeys.lastErrors, payload);
      renderErrors(payload);
      renderCacheSummary();
      setMessage("DTC snapshot updated.", "ok");
    });
  }

  async function clearErrors() {
    await runAction("/api/errors/clear", { method: "POST" }, async (payload) => {
      setMessage(`Clear DTC response frame=${payload.packet ? payload.packet.frame : "?"}.`, "ok");
      await loadErrors();
    });
  }

  async function loadHealth(silent) {
    await runAction("/api/health", {}, (payload) => {
      state.health = payload;
      writeJson(storageKeys.lastHealth, payload);
      renderHealth(payload);
      renderCacheSummary();
      if (!silent) setMessage("Health snapshot updated.", "ok");
    }, { silentBusy: true });
  }

  async function loadMemory() {
    const hi = encodeURIComponent(els.memHi.value.trim());
    const lo = encodeURIComponent(els.memLo.value.trim());
    const len = encodeURIComponent(els.memLen.value.trim());
    await runAction(`/api/memory/read?hi=${hi}&lo=${lo}&len=${len}`, {}, (payload) => {
      state.technical = payload;
      writeJson(storageKeys.lastTechnical, payload);
      renderTechnical(payload);
      renderCacheSummary();
      setMessage("Raw memory snapshot updated.", "ok");
    });
  }

  async function loadCatalog() {
    await runAction("/api/sensors/catalog", {}, (payload) => {
      state.technical = payload;
      writeJson(storageKeys.lastTechnical, payload);
      renderTechnical(payload);
      renderCacheSummary();
      setMessage("Catalog loaded.", "ok");
    }, { silentBusy: true });
  }

  async function runAction(url, options, onSuccess, settings) {
    const cfg = settings || {};
    if (state.requestInFlight) {
      if (!cfg.silentBusy) {
        setMessage("Browser request suppressed: another operation is already in flight.", "err");
      }
      return;
    }

    state.requestInFlight = true;
    syncControls();

    try {
      const response = await fetch(url, options);
      const payload = await response.json();

      if (!response.ok || payload.ok === false) {
        handleErrorPayload(payload);
        return;
      }

      if (payload.protocol_state) {
        onStatusResponse(payload);
      }

      onSuccess(payload);
    } catch (error) {
      setMessage(`Network error: ${error.message}`, "err");
    } finally {
      state.requestInFlight = false;
      syncControls();
      schedulePolling();
    }
  }

  function handleErrorPayload(payload) {
    if (payload && payload.protocol_state) {
      state.status = payload;
      writeJson(storageKeys.lastStatus, payload);
      renderAll();
    }

    const code = payload && payload.error_code ? payload.error_code : "request_failed";
    const message = payload && payload.message ? payload.message : "Request failed.";
    setMessage(`${code}: ${message}`, "err");
  }

  function setMessage(text, tone) {
    els.messageBox.className = `message ${tone || ""}`.trim();
    els.messageBox.textContent = text;
  }

  function describePolling(status) {
    if (!status || !status.protocol_state) {
      return "Polling status only";
    }
    if (status.protocol_state === "handshaking") {
      return "Handshake active: polling status only";
    }
    if (status.protocol_state === "busy") {
      return `Busy with ${status.current_operation}`;
    }
    if (status.protocol_state === "ready") {
      return state.activeTab === "live"
        ? "Ready: polling status and live sensors"
        : state.activeTab === "health"
          ? "Ready: polling status and health"
          : "Ready: polling status only";
    }
    return `Polling status while state=${status.protocol_state}`;
  }

  function messageTone(protocolState) {
    if (protocolState === "ready") return "ok";
    if (protocolState === "busy" || protocolState === "handshaking") return "warn";
    if (protocolState === "error") return "err";
    return "";
  }

  function escapeHtml(text) {
    return String(text)
      .replace(/&/g, "&amp;")
      .replace(/</g, "&lt;")
      .replace(/>/g, "&gt;")
      .replace(/"/g, "&quot;");
  }
})();
