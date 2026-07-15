(function () {
  const translations = {
    en: {
      pageTitle: "MA1.7 ESP32 Scanner",
      heroSubtitle: "Single-request browser client with protocol-state gating.",
      languageLabel: "Language",
      noLiveData: "No live data yet",
      linkIdle: "Link idle",
      linkOnline: "Link {ms} ms | {age}",
      linkOnlineFresh: "Link {ms} ms | ECU {age}",
      linkBusy: "Link busy: {operation}",
      linkOffline: "Link offline",
      linkRebooting: "Link rebooting",
      ageNow: "just now",
      ageSeconds: "{count}s ago",
      ageMinutes: "{count}m ago",
      cacheEmpty: "Cache empty",
      connectEcu: "Connect ECU",
      rebootEsp32: "Reboot ESP32",
      refreshNow: "Refresh now",
      pollingStatusOnly: "Polling status only",
      rebootingEsp32: "ESP32 rebooting. Waiting for it to come back.",
      waitingFirstPoll: "Waiting for first status poll.",
      sessionStatus: "Session Status",
      tabLive: "Live",
      tabDtc: "DTC",
      tabHealth: "Health",
      tabTechnical: "Technical",
      collectionTable: "Collection table",
      auto: "Auto",
      table1: "Table 1",
      table2: "Table 2",
      readLiveSensors: "Read live sensors",
      readDtc: "Read DTC",
      clearDtc: "Clear DTC",
      refreshHealth: "Refresh health",
      technicalWorkflow: "Technical tools stay outside the normal driving workflow.",
      addrHi: "Addr HI",
      addrLo: "Addr LO",
      length: "Length",
      readMemory: "Read memory",
      loadSensorCatalog: "Load sensor catalog",
      currentProtocolState: "Current Protocol State",
      sensorColumnRaw: "Raw",
      sensorColumnValue: "Value",
      statusUnavailable: "Status unavailable.",
      footerActiveRequest: "The browser never sends a second ECU request while another one is active.",
      cachedSnapshot: "Cached Snapshot",
      nothingStoredYet: "Nothing stored yet.",
      footerCache: "Local cache is only for UX and history. The ESP32 remains the source of truth.",
      metricProtocol: "Protocol",
      metricOperation: "Operation",
      metricConnected: "Connected",
      metricReady: "Ready",
      metricBusy: "Busy",
      metricErrorCode: "Error code",
      yes: "yes",
      no: "no",
      unknown: "unknown",
      none: "none",
      state: "state",
      busy: "busy",
      ready: "ready",
      op: "op",
      err: "err",
      initStringsPending: "Init strings become available only after ECU ready state.",
      noStatusResponse: "No status response yet.",
      cacheStoredPrefix: "Stored locally:",
      cacheStoredSuffix: "Cached values are never replayed as ECU commands.",
      technicalModeOn: "Technical mode on",
      technicalModeOff: "Technical mode off",
      technicalEnabledDetail: "Technical mode enabled. Raw memory and catalog are visible here.",
      technicalDisabledDetail: "Technical mode disabled. This tab stays for explicit technical access only.",
      noLiveSensorSnapshot: "No live sensor snapshot loaded yet.",
      sensorLabel: "Sensor",
      noDtcSnapshot: "No DTC snapshot loaded yet.",
      dtcPresent: "Present",
      dtcStored: "Stored",
      unknownDtc: "Unknown DTC",
      noHealthSnapshot: "No health snapshot loaded yet.",
      noTechnicalData: "No technical data loaded yet.",
      catalog: "Catalog",
      entries: "entries",
      packet: "Packet",
      statusRefreshed: "Status refreshed from ESP32.",
      requestStarted: "Sending {operation}...",
      requestFinished: "{operation} completed in {ms} ms.",
      rebootConfirm: "Reboot the ESP32 now?",
      rebootRequested: "Reboot command sent. Waiting for the ESP32 to come back.",
      liveSensorUpdated: "Live sensor snapshot updated.",
      dtcUpdated: "DTC snapshot updated.",
      clearDtcResponse: "Clear DTC response frame={frame}.",
      healthUpdated: "Health snapshot updated.",
      rawMemoryUpdated: "Raw memory snapshot updated.",
      catalogLoaded: "Catalog loaded.",
      technicalEnabled: "Technical mode enabled on ESP32.",
      technicalDisabled: "Technical mode disabled on ESP32.",
      browserRequestSuppressed: "Browser request suppressed: another operation is already in flight.",
      networkError: "Network error: {message}",
      requestFailed: "Request failed.",
      handshakeActive: "Handshake active: polling status only",
      rebootPendingPoll: "Reboot in progress: polling for ESP32 return",
      disconnectedReadyToConnect: "Disconnected: reconnect is available",
      statusLinePrimary: "Protocol {state}",
      statusLineSecondary: "Connected {connected} | Ready {ready} | Busy {busy}",
      statusLineTertiary: "Operation {operation} | Error {error}",
      busyWith: "Busy with {operation}",
      readyPollingLive: "Ready: polling status and live sensors",
      readyPollingHealth: "Ready: polling status and health",
      readyPollingOnly: "Ready: polling status only",
      pollingWhileState: "Polling status while state={state}",
      cacheStatus: "status",
      cacheSensors: "sensors",
      cacheDtc: "dtc",
      cacheHealth: "health",
      cacheTechnical: "technical",
      protocolStateBusy: "busy",
      protocolStateReady: "ready",
      protocolStateHandshaking: "handshaking",
      protocolStateError: "error",
      protocolStateDisconnected: "disconnected",
      operationReadErrors: "read errors",
      operationReadSensor: "read sensor",
      operationReadMemory: "read memory",
      operationReadCollection: "read collection",
      operationClearErrors: "clear errors",
      operationDetermineCollectionTable: "determine collection table",
      operationNone: "none",
      dataFresh: "Live data",
      dataCached: "Cached data",
      dataSession: "Session data",
      dataStale: "Stale cache",
      dataEmpty: "No snapshot",
      sectionLiveMetaFresh: "Live sensors updated {age}. Auto-refresh active.",
      sectionLiveMetaCached: "Showing cached live sensors from {age}.",
      sectionLiveMetaStale: "Showing stale live sensors from {age}. Refresh recommended.",
      sectionLiveMetaEmpty: "No live sensor snapshot loaded yet.",
      sectionDtcMetaFresh: "DTC snapshot updated {age}.",
      sectionDtcMetaCached: "Showing cached DTC snapshot from {age}.",
      sectionDtcMetaStale: "Showing stale cached DTC snapshot from {age}.",
      sectionDtcMetaEmpty: "No DTC snapshot loaded yet.",
      sectionHealthMetaFresh: "Health snapshot updated {age}. Auto-refresh active on this tab.",
      sectionHealthMetaCached: "Showing cached health snapshot from {age}.",
      sectionHealthMetaStale: "Showing stale cached health snapshot from {age}.",
      sectionHealthMetaEmpty: "No health snapshot loaded yet.",
      sectionTechnicalMetaFresh: "Technical data updated {age}.",
      sectionTechnicalMetaCached: "Showing cached technical data from {age}.",
      sectionTechnicalMetaStale: "Showing stale technical data from {age}.",
      sectionTechnicalMetaSession: "Technical memory read kept only for this browser session.",
      sectionTechnicalMetaEmpty: "No technical data loaded yet.",
      activeCollectionTable: "Active table {table}",
      activeCollectionTableUnknown: "Active table unknown",
      technicalMemorySessionHint: "Memory reads are session-only on this device."
    },
    "pt-BR": {
      pageTitle: "Scanner ESP32 MA1.7",
      heroSubtitle: "Cliente web de requisicao unica com bloqueio por estado do protocolo.",
      languageLabel: "Idioma",
      noLiveData: "Sem dados ao vivo ainda",
      linkIdle: "Link ocioso",
      linkOnline: "Link {ms} ms | {age}",
      linkOnlineFresh: "Link {ms} ms | ECU {age}",
      linkBusy: "Link ocupado: {operation}",
      linkOffline: "Link offline",
      linkRebooting: "Link reiniciando",
      ageNow: "agora",
      ageSeconds: "ha {count}s",
      ageMinutes: "ha {count}min",
      cacheEmpty: "Cache vazio",
      connectEcu: "Conectar ECU",
      rebootEsp32: "Reiniciar ESP32",
      refreshNow: "Atualizar agora",
      pollingStatusOnly: "Consultando apenas o status",
      rebootingEsp32: "ESP32 reiniciando. Aguardando ele voltar.",
      waitingFirstPoll: "Aguardando a primeira consulta de status.",
      sessionStatus: "Status da sessao",
      tabLive: "Ao vivo",
      tabDtc: "DTC",
      tabHealth: "Saude",
      tabTechnical: "Tecnico",
      collectionTable: "Tabela de coleta",
      auto: "Auto",
      table1: "Tabela 1",
      table2: "Tabela 2",
      readLiveSensors: "Ler sensores ao vivo",
      readDtc: "Ler DTC",
      clearDtc: "Limpar DTC",
      refreshHealth: "Atualizar saude",
      technicalWorkflow: "Ferramentas tecnicas ficam fora do fluxo normal de conducao.",
      addrHi: "End. HI",
      addrLo: "End. LO",
      length: "Tamanho",
      readMemory: "Ler memoria",
      loadSensorCatalog: "Carregar catalogo de sensores",
      currentProtocolState: "Estado atual do protocolo",
      sensorColumnRaw: "Bruto",
      sensorColumnValue: "Valor",
      statusUnavailable: "Status indisponivel.",
      footerActiveRequest: "O navegador nunca envia uma segunda requisicao para a ECU enquanto outra estiver ativa.",
      cachedSnapshot: "Snapshot em cache",
      nothingStoredYet: "Nada armazenado ainda.",
      footerCache: "O cache local serve apenas para UX e historico. O ESP32 continua sendo a fonte da verdade.",
      metricProtocol: "Protocolo",
      metricOperation: "Operacao",
      metricConnected: "Conectado",
      metricReady: "Pronto",
      metricBusy: "Ocupado",
      metricErrorCode: "Codigo de erro",
      yes: "sim",
      no: "nao",
      unknown: "desconhecido",
      none: "nenhuma",
      state: "estado",
      busy: "ocupado",
      ready: "pronto",
      op: "op",
      err: "erro",
      initStringsPending: "As strings de inicializacao ficam disponiveis somente depois de a ECU entrar em estado pronto.",
      noStatusResponse: "Ainda nao houve resposta de status.",
      cacheStoredPrefix: "Armazenado localmente:",
      cacheStoredSuffix: "Valores em cache nunca sao reenviados como comandos para a ECU.",
      technicalModeOn: "Modo tecnico ligado",
      technicalModeOff: "Modo tecnico desligado",
      technicalEnabledDetail: "Modo tecnico habilitado. Memoria bruta e catalogo ficam visiveis aqui.",
      technicalDisabledDetail: "Modo tecnico desabilitado. Esta aba fica reservada para acesso tecnico explicito.",
      noLiveSensorSnapshot: "Nenhum snapshot de sensores ao vivo foi carregado ainda.",
      sensorLabel: "Sensor",
      noDtcSnapshot: "Nenhum snapshot de DTC foi carregado ainda.",
      dtcPresent: "Presente",
      dtcStored: "Memorizado",
      unknownDtc: "DTC desconhecido",
      noHealthSnapshot: "Nenhum snapshot de saude foi carregado ainda.",
      noTechnicalData: "Nenhum dado tecnico foi carregado ainda.",
      catalog: "Catalogo",
      entries: "entradas",
      packet: "Pacote",
      statusRefreshed: "Status atualizado a partir do ESP32.",
      requestStarted: "Enviando {operation}...",
      requestFinished: "{operation} concluida em {ms} ms.",
      rebootConfirm: "Reiniciar o ESP32 agora?",
      rebootRequested: "Comando de reinicio enviado. Aguardando o ESP32 voltar.",
      liveSensorUpdated: "Snapshot de sensores ao vivo atualizado.",
      dtcUpdated: "Snapshot de DTC atualizado.",
      clearDtcResponse: "Resposta de limpeza de DTC frame={frame}.",
      healthUpdated: "Snapshot de saude atualizado.",
      rawMemoryUpdated: "Snapshot de memoria bruta atualizado.",
      catalogLoaded: "Catalogo carregado.",
      technicalEnabled: "Modo tecnico habilitado no ESP32.",
      technicalDisabled: "Modo tecnico desabilitado no ESP32.",
      browserRequestSuppressed: "Requisicao do navegador suprimida: outra operacao ja esta em andamento.",
      networkError: "Erro de rede: {message}",
      requestFailed: "Falha na requisicao.",
      handshakeActive: "Handshake ativo: consultando apenas o status",
      rebootPendingPoll: "Reinicio em andamento: consultando o retorno do ESP32",
      disconnectedReadyToConnect: "Desconectado: reconexao disponivel",
      statusLinePrimary: "Protocolo {state}",
      statusLineSecondary: "Conectado {connected} | Pronto {ready} | Ocupado {busy}",
      statusLineTertiary: "Operacao {operation} | Erro {error}",
      busyWith: "Ocupado com {operation}",
      readyPollingLive: "Pronto: consultando status e sensores ao vivo",
      readyPollingHealth: "Pronto: consultando status e saude",
      readyPollingOnly: "Pronto: consultando apenas o status",
      pollingWhileState: "Consultando status enquanto estado={state}",
      cacheStatus: "status",
      cacheSensors: "sensores",
      cacheDtc: "dtc",
      cacheHealth: "saude",
      cacheTechnical: "tecnico",
      protocolStateBusy: "ocupado",
      protocolStateReady: "pronto",
      protocolStateHandshaking: "handshake",
      protocolStateError: "erro",
      protocolStateDisconnected: "desconectado",
      operationReadErrors: "leitura de erros",
      operationReadSensor: "leitura de sensor",
      operationReadMemory: "leitura de memoria",
      operationReadCollection: "leitura de coleta",
      operationClearErrors: "limpeza de erros",
      operationDetermineCollectionTable: "determinacao da tabela de coleta",
      operationNone: "nenhuma",
      dataFresh: "Dado ao vivo",
      dataCached: "Dado em cache",
      dataSession: "Dado da sessao",
      dataStale: "Cache desatualizado",
      dataEmpty: "Sem snapshot",
      sectionLiveMetaFresh: "Sensores ao vivo atualizados {age}. Autoatualizacao ativa.",
      sectionLiveMetaCached: "Exibindo sensores em cache de {age}.",
      sectionLiveMetaStale: "Exibindo sensores desatualizados de {age}. Recomendado atualizar.",
      sectionLiveMetaEmpty: "Nenhum snapshot de sensores ao vivo foi carregado ainda.",
      sectionDtcMetaFresh: "Snapshot de DTC atualizado {age}.",
      sectionDtcMetaCached: "Exibindo snapshot de DTC em cache de {age}.",
      sectionDtcMetaStale: "Exibindo snapshot de DTC em cache desatualizado de {age}.",
      sectionDtcMetaEmpty: "Nenhum snapshot de DTC foi carregado ainda.",
      sectionHealthMetaFresh: "Snapshot de saude atualizado {age}. Autoatualizacao ativa nesta aba.",
      sectionHealthMetaCached: "Exibindo snapshot de saude em cache de {age}.",
      sectionHealthMetaStale: "Exibindo snapshot de saude em cache desatualizado de {age}.",
      sectionHealthMetaEmpty: "Nenhum snapshot de saude foi carregado ainda.",
      sectionTechnicalMetaFresh: "Dados tecnicos atualizados {age}.",
      sectionTechnicalMetaCached: "Exibindo dados tecnicos em cache de {age}.",
      sectionTechnicalMetaStale: "Exibindo dados tecnicos desatualizados de {age}.",
      sectionTechnicalMetaSession: "Leitura de memoria tecnica mantida apenas nesta sessao do navegador.",
      sectionTechnicalMetaEmpty: "Nenhum dado tecnico foi carregado ainda.",
      activeCollectionTable: "Tabela ativa {table}",
      activeCollectionTableUnknown: "Tabela ativa desconhecida",
      technicalMemorySessionHint: "Leituras de memoria ficam apenas nesta sessao do dispositivo."
    }
  };

  const CACHE_VERSION = 2;
  const dataTtls = {
    status: 5000,
    live: 8000,
    dtc: 60000,
    health: 15000,
    catalog: 86400000,
    technical: 300000
  };

  const storageKeys = {
    prefLocale: "ma17.pref.locale",
    prefTab: "ma17.pref.activeTab",
    prefCollectionTable: "ma17.pref.collectionTable",
    cacheStatus: "ma17.cache.status",
    cacheLive: "ma17.cache.live",
    cacheDtc: "ma17.cache.dtc",
    cacheHealth: "ma17.cache.health",
    cacheCatalog: "ma17.cache.catalog",
    sessionTechnicalMemory: "ma17.session.technicalMemory"
  };

  const initialLocale = detectInitialLocale();
  const initialActiveTab = localStorage.getItem(storageKeys.prefTab) || "live";
  const initialCollectionTable = localStorage.getItem(storageKeys.prefCollectionTable) || "0";
  const initialStatusEntry = loadPersistentEntry(getLocalizedStorageKey(storageKeys.cacheStatus, initialLocale));
  const initialLiveEntry = loadPersistentEntry(getLocalizedStorageKey(storageKeys.cacheLive, initialLocale));
  const initialDtcEntry = loadPersistentEntry(getLocalizedStorageKey(storageKeys.cacheDtc, initialLocale));
  const initialHealthEntry = loadPersistentEntry(getLocalizedStorageKey(storageKeys.cacheHealth, initialLocale));
  const initialCatalogEntry = loadPersistentEntry(getLocalizedStorageKey(storageKeys.cacheCatalog, initialLocale));
  const initialTechnicalSessionEntry = loadSessionEntry(storageKeys.sessionTechnicalMemory);

  const state = {
    activeTab: initialActiveTab,
    locale: initialLocale,
    collectionTable: initialCollectionTable,
    technicalMode: Boolean(
      getEntryPayload(initialStatusEntry) &&
      getEntryPayload(initialStatusEntry).technical_mode_enabled
    ),
    requestInFlight: false,
    rebootPendingUntil: 0,
    lastRequestStartedAt: 0,
    lastRoundTripMs: null,
    lastSuccessAt: 0,
    backendReachable: false,
    activeRequestLabel: "",
    pollTimer: null,
    data: {
      status: initialStatusEntry,
      live: initialLiveEntry,
      dtc: initialDtcEntry,
      health: initialHealthEntry,
      technical: chooseMostRecentEntry(initialTechnicalSessionEntry, initialCatalogEntry)
    }
  };

  const els = {
    title: document.querySelector(".title"),
    subtitle: document.querySelector(".subtitle"),
    liveBadge: byId("liveBadge"),
    netBadge: byId("netBadge"),
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
    liveMeta: byId("liveMeta"),
    dtcMeta: byId("dtcMeta"),
    healthMeta: byId("healthMeta"),
    technicalMeta: byId("technicalMeta"),
    connectBtn: byId("connectBtn"),
    rebootBtn: byId("rebootBtn"),
    refreshBtn: byId("refreshBtn"),
    technicalToggleBtn: byId("technicalToggleBtn"),
    loadSensorsBtn: byId("loadSensorsBtn"),
    readErrorsBtn: byId("readErrorsBtn"),
    clearErrorsBtn: byId("clearErrorsBtn"),
    loadHealthBtn: byId("loadHealthBtn"),
    readMemoryBtn: byId("readMemoryBtn"),
    loadCatalogBtn: byId("loadCatalogBtn"),
    languageLabel: byId("languageLabel"),
    languageSelect: byId("languageSelect"),
    sessionStatusHeading: document.querySelector("main .card h2"),
    tabLiveButton: document.querySelector('.tab[data-tab="live"]'),
    tabDtcButton: document.querySelector('.tab[data-tab="dtc"]'),
    tabHealthButton: document.querySelector('.tab[data-tab="health"]'),
    tabTechnicalButton: document.querySelector('.tab[data-tab="technical"]'),
    tabButtons: Array.from(document.querySelectorAll(".tab")),
    collectionTableLabel: document.querySelector('label[for="tableSelect"]'),
    tableAutoOption: document.querySelector('#tableSelect option[value="0"]'),
    table1Option: document.querySelector('#tableSelect option[value="1"]'),
    table2Option: document.querySelector('#tableSelect option[value="2"]'),
    sensorTableHead: document.querySelector(".table-head"),
    technicalSummary: byId("tab-technical").querySelector(".small"),
    memHiLabel: document.querySelector('label[for="memHi"]'),
    memLoLabel: document.querySelector('label[for="memLo"]'),
    memLenLabel: document.querySelector('label[for="memLen"]'),
    currentProtocolHeading: document.querySelector("aside .card h3"),
    currentProtocolFooter: document.querySelectorAll(".footer-note")[0],
    cachedSnapshotHeading: document.querySelectorAll("aside .card h3")[1],
    cachedSnapshotFooter: document.querySelectorAll(".footer-note")[1],
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

  function normalizeLocale(locale) {
    const raw = String(locale || "").trim().toLowerCase().replace("_", "-");
    return raw.startsWith("pt") ? "pt-BR" : "en";
  }

  function detectInitialLocale() {
    const stored = localStorage.getItem(storageKeys.prefLocale);
    if (stored) {
      return normalizeLocale(stored);
    }
    return normalizeLocale(navigator.language || navigator.userLanguage || "en");
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

  function getLocalizedStorageKey(baseKey, locale) {
    return `${baseKey}.${normalizeLocale(locale || state.locale)}`;
  }

  function removeJson(key, storage) {
    try {
      (storage || localStorage).removeItem(key);
    } catch (_) {
      // Ignore storage failures.
    }
  }

  function makeEntry(payload, options) {
    const cfg = options || {};
    return {
      version: CACHE_VERSION,
      savedAt: Date.now(),
      ttlMs: cfg.ttlMs || 0,
      source: cfg.source || "live",
      sessionOnly: Boolean(cfg.sessionOnly),
      payload
    };
  }

  function loadPersistentEntry(key) {
    const entry = readJson(key);
    const normalized = normalizeEntry(entry);
    return normalized
      ? Object.assign({}, normalized, { source: "cache", sessionOnly: false })
      : null;
  }

  function savePersistentEntry(key, payload, ttlMs, source) {
    writeJson(key, makeEntry(payload, { ttlMs, source: source || "live" }));
  }

  function loadSessionEntry(key) {
    try {
      const raw = sessionStorage.getItem(key);
      const normalized = normalizeEntry(raw ? JSON.parse(raw) : null);
      return normalized
        ? Object.assign({}, normalized, { source: "session", sessionOnly: true })
        : null;
    } catch (_) {
      return null;
    }
  }

  function saveSessionEntry(key, payload, source) {
    try {
      sessionStorage.setItem(
        key,
        JSON.stringify(makeEntry(payload, {
          ttlMs: dataTtls.technical,
          source: source || "session",
          sessionOnly: true
        }))
      );
    } catch (_) {
      // Ignore storage failures.
    }
  }

  function normalizeEntry(entry) {
    if (!entry || entry.version !== CACHE_VERSION || typeof entry.savedAt !== "number") {
      return null;
    }
    return entry;
  }

  function chooseMostRecentEntry(left, right) {
    if (!left) {
      return right || null;
    }
    if (!right) {
      return left;
    }
    return left.savedAt >= right.savedAt ? left : right;
  }

  function isEntryExpired(entry) {
    if (!entry) {
      return true;
    }
    if (!entry.ttlMs) {
      return false;
    }
    return Date.now() > entry.savedAt + entry.ttlMs;
  }

  function getEntryPayload(entry) {
    return entry && entry.payload ? entry.payload : null;
  }

  function getDataBadge(entry) {
    if (!entry) {
      return t("dataEmpty");
    }
    if (entry.sessionOnly) {
      return t("dataSession");
    }
    if (isEntryExpired(entry)) {
      return t("dataStale");
    }
    return entry.source === "live" ? t("dataFresh") : t("dataCached");
  }

  function setDataEntry(name, payload, options) {
    const cfg = options || {};
    const entry = makeEntry(payload, {
      ttlMs: cfg.ttlMs || 0,
      source: cfg.source || "live",
      sessionOnly: Boolean(cfg.sessionOnly)
    });
    state.data[name] = entry;

    if (cfg.sessionKey) {
      saveSessionEntry(cfg.sessionKey, payload, cfg.source);
    } else if (cfg.storageKey) {
      savePersistentEntry(cfg.storageKey, payload, cfg.ttlMs || 0, cfg.source);
    }

    return entry;
  }

  function clearDataEntry(name, options) {
    const cfg = options || {};
    state.data[name] = null;
    if (cfg.storageKey) {
      removeJson(cfg.storageKey, localStorage);
    }
    if (cfg.sessionKey) {
      removeJson(cfg.sessionKey, sessionStorage);
    }
  }

  function t(key, replacements) {
    const localeTable = translations[state.locale] || translations.en;
    let text = localeTable[key] || translations.en[key] || key;
    if (!replacements) {
      return text;
    }

    Object.keys(replacements).forEach((name) => {
      text = text.replace(`{${name}}`, String(replacements[name]));
    });
    return text;
  }

  function translateProtocolState(protocolState) {
    const map = {
      busy: "protocolStateBusy",
      ready: "protocolStateReady",
      handshaking: "protocolStateHandshaking",
      error: "protocolStateError",
      disconnected: "protocolStateDisconnected"
    };
    return t(map[protocolState] || "unknown");
  }

  function translateOperation(operation) {
    const map = {
      read_errors: "operationReadErrors",
      read_sensor: "operationReadSensor",
      read_memory: "operationReadMemory",
      read_collection: "operationReadCollection",
      clear_errors: "operationClearErrors",
      determine_collection_table: "operationDetermineCollectionTable",
      none: "operationNone"
    };
    return t(map[operation] || "unknown");
  }

  function setStaticText() {
    document.documentElement.lang = state.locale;
    document.title = t("pageTitle");
    els.title.textContent = t("pageTitle");
    els.subtitle.textContent = t("heroSubtitle");
    els.languageLabel.textContent = t("languageLabel");
    els.languageSelect.value = state.locale;
    els.tableSelect.value = state.collectionTable;
    els.connectBtn.textContent = t("connectEcu");
    els.rebootBtn.textContent = t("rebootEsp32");
    els.refreshBtn.textContent = t("refreshNow");
    els.sessionStatusHeading.textContent = t("sessionStatus");
    els.tabLiveButton.textContent = t("tabLive");
    els.tabDtcButton.textContent = t("tabDtc");
    els.tabHealthButton.textContent = t("tabHealth");
    els.tabTechnicalButton.textContent = t("tabTechnical");
    els.collectionTableLabel.textContent = t("collectionTable");
    els.tableAutoOption.textContent = t("auto");
    els.table1Option.textContent = t("table1");
    els.table2Option.textContent = t("table2");
    if (els.sensorTableHead) {
      const columns = els.sensorTableHead.children;
      columns[0].textContent = t("sensorLabel");
      columns[1].textContent = t("sensorColumnRaw");
      columns[2].textContent = t("sensorColumnValue");
    }
    els.loadSensorsBtn.textContent = t("readLiveSensors");
    els.readErrorsBtn.textContent = t("readDtc");
    els.clearErrorsBtn.textContent = t("clearDtc");
    els.loadHealthBtn.textContent = t("refreshHealth");
    els.memHiLabel.textContent = t("addrHi");
    els.memLoLabel.textContent = t("addrLo");
    els.memLenLabel.textContent = t("length");
    els.readMemoryBtn.textContent = t("readMemory");
    els.loadCatalogBtn.textContent = t("loadSensorCatalog");
    els.currentProtocolHeading.textContent = t("currentProtocolState");
    els.currentProtocolFooter.textContent = t("footerActiveRequest");
    els.cachedSnapshotHeading.textContent = t("cachedSnapshot");
    els.cachedSnapshotFooter.textContent = t("footerCache");
  }

  function bindEvents() {
    document.querySelectorAll(".tab").forEach((button) => {
      button.addEventListener("click", () => setTab(button.dataset.tab));
    });

    els.languageSelect.addEventListener("change", onLocaleChange);
    els.tableSelect.addEventListener("change", onCollectionTableChange);

    els.connectBtn.addEventListener("click", () =>
      runAction("/api/connect", { method: "POST" }, onStatusResponse));
    els.rebootBtn.addEventListener("click", rebootEsp32);
    els.refreshBtn.addEventListener("click", () => pollStatus(true));
    els.technicalToggleBtn.addEventListener("click", toggleTechnicalMode);
    els.loadSensorsBtn.addEventListener("click", loadSensors);
    els.readErrorsBtn.addEventListener("click", loadErrors);
    els.clearErrorsBtn.addEventListener("click", clearErrors);
    els.loadHealthBtn.addEventListener("click", loadHealth);
    els.readMemoryBtn.addEventListener("click", loadMemory);
    els.loadCatalogBtn.addEventListener("click", loadCatalog);
  }

  function setTab(tab) {
    state.activeTab = tab;
    localStorage.setItem(storageKeys.prefTab, tab);
    renderTabs();
    schedulePolling();
  }

  function onCollectionTableChange() {
    state.collectionTable = els.tableSelect.value;
    localStorage.setItem(storageKeys.prefCollectionTable, state.collectionTable);
    renderSectionMeta();
  }

  function renderTabs() {
    els.tabButtons.forEach((button) => {
      button.classList.toggle("active", button.dataset.tab === state.activeTab);
    });

    ["live", "dtc", "health", "technical"].forEach((name) => {
      byId(`tab-${name}`).classList.toggle("section-hidden", state.activeTab !== name);
    });
  }

  function renderAll() {
    setStaticText();
    renderTabs();
    renderStatus();
    renderNetworkBadge();
    renderCacheSummary();
    renderTechnicalMode();
    renderCachedSections();
    renderSectionMeta();
    syncControls();
  }

  function renderStatus() {
    const status = getEntryPayload(state.data.status);
    if (!status) {
      els.statusGrid.innerHTML = "";
      els.liveBadge.textContent = t("noLiveData");
      els.pollInfo.textContent = isRebootPending() ? t("rebootPendingPoll") : t("pollingStatusOnly");
      els.stateSummary.textContent = t("noStatusResponse");
      setMessage(isRebootPending() ? t("rebootingEsp32") : t("waitingFirstPoll"));
      return;
    }

    const metrics = [
      [t("metricProtocol"), translateProtocolState(status.protocol_state || "unknown")],
      [t("metricOperation"), translateOperation(status.current_operation || "none")],
      [t("metricConnected"), status.connected ? t("yes") : t("no")],
      [t("metricReady"), status.init_ready ? t("yes") : t("no")],
      [t("metricBusy"), status.busy ? t("yes") : t("no")],
      [t("metricErrorCode"), String(status.error_code ?? 0)]
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
      : `<div class="list-item small">${escapeHtml(t("initStringsPending"))}</div>`;

    els.stateSummary.className = `message ${messageTone(status.protocol_state)}`;
    els.stateSummary.textContent = [
      t("statusLinePrimary", { state: translateProtocolState(status.protocol_state) }),
      t("statusLineSecondary", {
        connected: status.connected ? t("yes") : t("no"),
        ready: status.init_ready ? t("yes") : t("no"),
        busy: status.busy ? t("yes") : t("no")
      }),
      t("statusLineTertiary", {
        operation: translateOperation(status.current_operation),
        error: status.error_code ?? 0
      })
    ].join("\n");

    els.liveBadge.textContent = `ECU ${translateProtocolState(status.protocol_state).toUpperCase()}`;

    els.pollInfo.textContent = describePolling(status);
  }

  function renderCacheSummary() {
    const pieces = [];
    const status = getEntryPayload(state.data.status);
    const live = getEntryPayload(state.data.live);
    const dtc = getEntryPayload(state.data.dtc);
    const health = getEntryPayload(state.data.health);
    const technical = getEntryPayload(state.data.technical);

    if (status) {
      pieces.push(`${t("cacheStatus")}:${translateProtocolState(status.protocol_state)}`);
    }
    if (live && live.sensors) pieces.push(`${t("cacheSensors")}:${live.sensors.length}`);
    if (dtc && dtc.errors) pieces.push(`${t("cacheDtc")}:${dtc.errors.length}`);
    if (health && health.health) pieces.push(`${t("cacheHealth")}:1`);
    if (technical) pieces.push(`${t("cacheTechnical")}:${getDataBadge(state.data.technical)}`);

    if (!pieces.length) {
      els.cacheBadge.textContent = t("cacheEmpty");
      els.cacheSummary.textContent = t("nothingStoredYet");
      return;
    }

    els.cacheBadge.textContent = `Cache ${pieces.join(" | ")}`;
    els.cacheSummary.textContent =
      `${t("cacheStoredPrefix")} ${pieces.join(", ")}. ${t("cacheStoredSuffix")}`;
  }

  function renderNetworkBadge() {
    const status = getEntryPayload(state.data.status);
    const meta = status && status.meta ? status.meta : null;

    if (isRebootPending()) {
      els.netBadge.textContent = t("linkRebooting");
      return;
    }

    if (state.requestInFlight && state.activeRequestLabel) {
      els.netBadge.textContent = t("linkBusy", { operation: state.activeRequestLabel });
      return;
    }

    if (state.backendReachable && state.lastRoundTripMs !== null) {
      if (meta && typeof meta.last_packet_age_ms === "number") {
        els.netBadge.textContent = t("linkOnlineFresh", {
          ms: state.lastRoundTripMs,
          age: formatElapsedMs(meta.last_packet_age_ms)
        });
      } else {
        els.netBadge.textContent = t("linkOnline", {
          ms: state.lastRoundTripMs,
          age: formatAge(state.lastSuccessAt)
        });
      }
      return;
    }

    els.netBadge.textContent = state.lastSuccessAt ? t("linkOffline") : t("linkIdle");
  }

  function renderTechnicalMode() {
    els.technicalToggleBtn.textContent = state.technicalMode
      ? t("technicalModeOn")
      : t("technicalModeOff");
    els.technicalSummary.textContent = state.technicalMode
      ? t("technicalEnabledDetail")
      : t("technicalDisabledDetail");
  }

  function renderCachedSections() {
    renderSensors(getEntryPayload(state.data.live));
    renderErrors(getEntryPayload(state.data.dtc));
    renderHealth(getEntryPayload(state.data.health));
    renderTechnical(getEntryPayload(state.data.technical));
  }

  function renderSectionMeta() {
    els.liveMeta.textContent = describeSectionMeta("live", state.data.live);
    els.dtcMeta.textContent = describeSectionMeta("dtc", state.data.dtc);
    els.healthMeta.textContent = describeSectionMeta("health", state.data.health);
    els.technicalMeta.textContent = describeSectionMeta("technical", state.data.technical);
  }

  function describeSectionMeta(section, entry) {
    if (!entry) {
      if (section === "live") return t("sectionLiveMetaEmpty");
      if (section === "dtc") return t("sectionDtcMetaEmpty");
      if (section === "health") return t("sectionHealthMetaEmpty");
      return t("sectionTechnicalMetaEmpty");
    }

    const age = formatAge(entry.savedAt);
    if (section === "live") {
      const payload = getEntryPayload(entry);
      const tableInfo =
        payload && payload.table
          ? ` ${t("activeCollectionTable", { table: payload.table })}.`
          : ` ${t("activeCollectionTableUnknown")}.`;
      if (isEntryExpired(entry)) return `${t("sectionLiveMetaStale", { age })}${tableInfo}`;
      if (entry.source === "live") return `${t("sectionLiveMetaFresh", { age })}${tableInfo}`;
      return `${t("sectionLiveMetaCached", { age })}${tableInfo}`;
    }

    if (section === "dtc") {
      if (isEntryExpired(entry)) return t("sectionDtcMetaStale", { age });
      if (entry.source === "live") return t("sectionDtcMetaFresh", { age });
      return t("sectionDtcMetaCached", { age });
    }

    if (section === "health") {
      if (isEntryExpired(entry)) return t("sectionHealthMetaStale", { age });
      if (entry.source === "live") return t("sectionHealthMetaFresh", { age });
      return t("sectionHealthMetaCached", { age });
    }

    if (entry.sessionOnly) {
      return `${t("sectionTechnicalMetaSession")} ${t("technicalMemorySessionHint")}`;
    }
    if (isEntryExpired(entry)) return t("sectionTechnicalMetaStale", { age });
    if (entry.source === "live") return t("sectionTechnicalMetaFresh", { age });
    return t("sectionTechnicalMetaCached", { age });
  }

  function renderSensors(payload) {
    const sensors = payload && Array.isArray(payload.sensors) ? payload.sensors : [];
    if (!sensors.length) {
      els.sensorList.innerHTML = `<div class="list-item small">${escapeHtml(t("noLiveSensorSnapshot"))}</div>`;
      return;
    }

    els.sensorList.innerHTML = sensors.map((sensor) => `
      <div class="table-row">
        <div>
          <strong>${escapeHtml(sensor.name || sensor.key || `${t("sensorLabel")} ${sensor.id}`)}</strong>
          <div class="small">id=${sensor.id} sub=${sensor.subcommand} slot=${sensor.slot}</div>
        </div>
        <div>
          <div class="mobile-label">${escapeHtml(t("sensorColumnRaw"))}</div>
          <div class="mono">${escapeHtml(String(sensor.raw))}</div>
        </div>
        <div>
          <div class="mobile-label">${escapeHtml(t("sensorColumnValue"))}</div>
          <div>${sensor.value !== undefined ? escapeHtml(String(sensor.value)) : "-"} ${escapeHtml(sensor.unit || "")}</div>
        </div>
      </div>
    `).join("");
  }

  function renderErrors(payload) {
    const errors = payload && Array.isArray(payload.errors) ? payload.errors : [];
    if (!errors.length) {
      els.errorList.innerHTML = `<div class="list-item small">${escapeHtml(t("noDtcSnapshot"))}</div>`;
      return;
    }

    els.errorList.innerHTML = errors.map((entry) => `
      <div class="list-item">
        <div><strong class="${entry.present ? "err" : "warn"}">${entry.present ? t("dtcPresent") : t("dtcStored")}</strong></div>
        <div>${escapeHtml(entry.description || t("unknownDtc"))}</div>
        <div class="small mono">frame=${entry.packet ? entry.packet.frame : "?"}</div>
      </div>
    `).join("");
  }

  function renderHealth(payload) {
    const health = payload && payload.health ? payload.health : null;
    if (!health) {
      els.healthList.innerHTML = `<div class="list-item small">${escapeHtml(t("noHealthSnapshot"))}</div>`;
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
      els.technicalList.innerHTML = `<div class="list-item small">${escapeHtml(t("noTechnicalData"))}</div>`;
      return;
    }

    if (payload.sensors) {
      els.technicalList.innerHTML = `
        <div class="list-item">
          <strong>${escapeHtml(t("catalog"))}</strong>
          <div class="small">${escapeHtml(t("entries"))}=${payload.sensors.length}</div>
          <div class="mono">${escapeHtml(payload.sensors.slice(0, 10).map((s) => s.key).join(", "))}</div>
        </div>
      `;
      return;
    }

    if (payload.packets) {
      els.technicalList.innerHTML = payload.packets.map((packet, index) => `
        <div class="list-item">
          <strong>${escapeHtml(t("packet"))} ${index + 1}</strong>
          <div class="mono">${escapeHtml(JSON.stringify(packet))}</div>
        </div>
      `).join("");
    }
  }

  function syncControls() {
    const status = getEntryPayload(state.data.status) || {};
    const ready = Boolean(
      status.meta && typeof status.meta.ready_for_commands === "boolean"
        ? status.meta.ready_for_commands
        : status.protocol_state === "ready" && status.init_ready && !status.busy
    );
    const technicalEnabled = state.technicalMode;
    const rebootPending = isRebootPending();
    const connectLocked = state.requestInFlight ||
      rebootPending ||
      status.protocol_state === "handshaking" ||
      status.busy ||
      status.connected;

    els.connectBtn.disabled = connectLocked;
    els.rebootBtn.disabled = state.requestInFlight || rebootPending;
    els.refreshBtn.disabled = state.requestInFlight || rebootPending;
    els.technicalToggleBtn.disabled = state.requestInFlight || rebootPending;
    els.languageSelect.disabled = state.requestInFlight || rebootPending;
    els.loadSensorsBtn.disabled = state.requestInFlight || rebootPending || !ready;
    els.readErrorsBtn.disabled = state.requestInFlight || rebootPending || !ready;
    els.clearErrorsBtn.disabled = state.requestInFlight || rebootPending || !ready;
    els.loadHealthBtn.disabled = state.requestInFlight || rebootPending;
    els.readMemoryBtn.disabled = state.requestInFlight || rebootPending || !ready || !technicalEnabled;
    els.loadCatalogBtn.disabled = state.requestInFlight || rebootPending || !technicalEnabled;
  }

  async function pollStatus(forceMessage) {
    await runAction("/api/status", {}, (payload) => {
      onStatusResponse(payload);
      if (forceMessage) {
        setMessage(t("statusRefreshed"), "ok");
      }
    }, {
      silentBusy: true,
      silentStartMessage: true,
      silentNetworkError: Date.now() < state.rebootPendingUntil
    });
    schedulePolling();
  }

  function onStatusResponse(payload) {
    state.rebootPendingUntil = 0;
    if (typeof payload.technical_mode_enabled === "boolean") {
      state.technicalMode = payload.technical_mode_enabled;
    }
    setDataEntry("status", payload, {
      storageKey: getLocalizedStorageKey(storageKeys.cacheStatus),
      ttlMs: dataTtls.status,
      source: "live"
    });
    renderAll();
  }

  function schedulePolling() {
    clearTimeout(state.pollTimer);
    const status = getEntryPayload(state.data.status) || {};
    if (state.requestInFlight) {
      return;
    }

    let delay = status.protocol_state === "ready" ? 2500 : 1500;
    if (Date.now() < state.rebootPendingUntil) {
      delay = Math.max(1000, state.rebootPendingUntil - Date.now());
    }
    state.pollTimer = setTimeout(async () => {
      await pollStatus(false);

      const latest = getEntryPayload(state.data.status) || {};
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
      setDataEntry("live", payload, {
        storageKey: getLocalizedStorageKey(storageKeys.cacheLive),
        ttlMs: dataTtls.live,
        source: "live"
      });
      renderSensors(payload);
      renderCacheSummary();
      renderSectionMeta();
      if (!silent) setMessage(t("liveSensorUpdated"), "ok");
    }, { silentStartMessage: Boolean(silent) });
  }

  async function loadErrors() {
    await runAction("/api/errors", {}, (payload) => {
      setDataEntry("dtc", payload, {
        storageKey: getLocalizedStorageKey(storageKeys.cacheDtc),
        ttlMs: dataTtls.dtc,
        source: "live"
      });
      renderErrors(payload);
      renderCacheSummary();
      renderSectionMeta();
      setMessage(t("dtcUpdated"), "ok");
    });
  }

  async function clearErrors() {
    const payload = await runAction("/api/errors/clear", { method: "POST" }, (payload) => {
      setMessage(t("clearDtcResponse", { frame: payload.packet ? payload.packet.frame : "?" }), "ok");
    });
    if (payload) {
      await loadErrors();
    }
  }

  async function loadHealth(silent) {
    await runAction("/api/health", {}, (payload) => {
      setDataEntry("health", payload, {
        storageKey: getLocalizedStorageKey(storageKeys.cacheHealth),
        ttlMs: dataTtls.health,
        source: "live"
      });
      renderHealth(payload);
      renderCacheSummary();
      renderSectionMeta();
      if (!silent) setMessage(t("healthUpdated"), "ok");
    }, {
      silentBusy: true,
      silentStartMessage: Boolean(silent)
    });
  }

  async function loadMemory() {
    const hi = encodeURIComponent(els.memHi.value.trim());
    const lo = encodeURIComponent(els.memLo.value.trim());
    const len = encodeURIComponent(els.memLen.value.trim());
    await runAction(`/api/memory/read?hi=${hi}&lo=${lo}&len=${len}`, {}, (payload) => {
      setDataEntry("technical", payload, {
        sessionKey: storageKeys.sessionTechnicalMemory,
        ttlMs: dataTtls.technical,
        source: "live",
        sessionOnly: true
      });
      renderTechnical(payload);
      renderCacheSummary();
      renderSectionMeta();
      setMessage(t("rawMemoryUpdated"), "ok");
    });
  }

  async function loadCatalog() {
    await runAction("/api/sensors/catalog", {}, (payload) => {
      setDataEntry("technical", payload, {
        storageKey: getLocalizedStorageKey(storageKeys.cacheCatalog),
        ttlMs: dataTtls.catalog,
        source: "live"
      });
      renderTechnical(payload);
      renderCacheSummary();
      renderSectionMeta();
      setMessage(t("catalogLoaded"), "ok");
    }, {
      silentBusy: true,
      silentStartMessage: true
    });
  }

  async function toggleTechnicalMode() {
    const desired = !state.technicalMode;
    const enabled = encodeURIComponent(desired ? "1" : "0");
    await runAction(`/api/technical-mode?enabled=${enabled}`, { method: "POST" }, (payload) => {
      const backendEnabled = Boolean(payload.technical_mode_enabled);
      state.technicalMode = backendEnabled;
      renderAll();
      setMessage(
        backendEnabled
          ? t("technicalEnabled")
          : t("technicalDisabled"),
        "ok"
      );
    });
  }

  async function rebootEsp32() {
    if (!window.confirm(t("rebootConfirm"))) {
      return;
    }

    const payload = await runAction("/api/reboot", { method: "POST" }, () => {
      setMessage(t("rebootRequested"), "ok");
    }, {
      silentBusy: true,
      silentStartMessage: true
    });

    if (!payload) {
      return;
    }

    state.rebootPendingUntil = Date.now() + 8000;
    syncControls();
    clearTimeout(state.pollTimer);
    schedulePolling();
  }

  async function runAction(url, options, onSuccess, settings) {
    const cfg = settings || {};
    if (state.requestInFlight) {
      if (!cfg.silentBusy) {
        setMessage(t("browserRequestSuppressed"), "err");
      }
      return null;
    }

    state.requestInFlight = true;
    state.lastRequestStartedAt = Date.now();
    state.activeRequestLabel = describeRequest(url);
    renderNetworkBadge();
    if (!cfg.silentStartMessage) {
      setMessage(t("requestStarted", { operation: state.activeRequestLabel }));
    }
    syncControls();

    try {
      const response = await fetch(buildApiUrl(url), options);
      const payload = await response.json();
      const elapsedMs = Math.max(1, Date.now() - state.lastRequestStartedAt);

      if (!response.ok || payload.ok === false) {
        state.backendReachable = true;
        state.lastRoundTripMs = elapsedMs;
        state.lastSuccessAt = Date.now();
        renderNetworkBadge();
        handleErrorPayload(payload);
        return null;
      }

      state.backendReachable = true;
      state.lastRoundTripMs = elapsedMs;
      state.lastSuccessAt = Date.now();

      if (payload.protocol_state) {
        onStatusResponse(payload);
      }

      onSuccess(payload);
      if (cfg.successMessageKey) {
        setMessage(t(cfg.successMessageKey), "ok");
      } else if (cfg.showFinishedMessage) {
        setMessage(t("requestFinished", { operation: state.activeRequestLabel, ms: elapsedMs }), "ok");
      }
      renderNetworkBadge();
      return payload;
    } catch (error) {
      state.backendReachable = false;
      renderNetworkBadge();
      if (!cfg.silentNetworkError) {
        setMessage(t("networkError", { message: error.message }), "err");
      }
      return null;
    } finally {
      state.requestInFlight = false;
      state.activeRequestLabel = "";
      renderNetworkBadge();
      syncControls();
      schedulePolling();
    }
  }

  function handleErrorPayload(payload) {
    if (payload && payload.protocol_state) {
      setDataEntry("status", payload, {
        storageKey: getLocalizedStorageKey(storageKeys.cacheStatus),
        ttlMs: dataTtls.status,
        source: "live"
      });
      renderAll();
    }

    const code = payload && payload.error_code ? payload.error_code : "request_failed";
    const message = payload && payload.message ? payload.message : t("requestFailed");
    setMessage(`${code}: ${message}`, "err");
  }

  function setMessage(text, tone) {
    els.messageBox.className = `message ${tone || ""}`.trim();
    els.messageBox.textContent = text;
  }

  function describePolling(status) {
    if (isRebootPending()) {
      return t("rebootPendingPoll");
    }
    if (!status || !status.protocol_state) {
      return t("pollingStatusOnly");
    }
    if (status.protocol_state === "handshaking") {
      return t("handshakeActive");
    }
    if (status.protocol_state === "busy") {
      return t("busyWith", { operation: translateOperation(status.current_operation) });
    }
    if (status.protocol_state === "ready") {
      return state.activeTab === "live"
        ? t("readyPollingLive")
        : state.activeTab === "health"
          ? t("readyPollingHealth")
          : t("readyPollingOnly");
    }
    if (status.protocol_state === "disconnected") {
      return t("disconnectedReadyToConnect");
    }
    return t("pollingWhileState", { state: translateProtocolState(status.protocol_state) });
  }

  function buildApiUrl(url) {
    if (!url.startsWith("/api/")) {
      return url;
    }

    const separator = url.indexOf("?") >= 0 ? "&" : "?";
    return `${url}${separator}lang=${encodeURIComponent(state.locale)}`;
  }

  async function onLocaleChange() {
    const nextLocale = normalizeLocale(els.languageSelect.value);
    if (nextLocale === state.locale) {
      return;
    }

    state.locale = nextLocale;
    localStorage.setItem(storageKeys.prefLocale, state.locale);
    state.data.status = loadPersistentEntry(getLocalizedStorageKey(storageKeys.cacheStatus, state.locale));
    state.data.live = loadPersistentEntry(getLocalizedStorageKey(storageKeys.cacheLive, state.locale));
    state.data.dtc = loadPersistentEntry(getLocalizedStorageKey(storageKeys.cacheDtc, state.locale));
    state.data.health = loadPersistentEntry(getLocalizedStorageKey(storageKeys.cacheHealth, state.locale));
    state.data.technical = chooseMostRecentEntry(
      loadSessionEntry(storageKeys.sessionTechnicalMemory),
      loadPersistentEntry(getLocalizedStorageKey(storageKeys.cacheCatalog, state.locale))
    );
    state.technicalMode = Boolean(
      getEntryPayload(state.data.status) &&
      getEntryPayload(state.data.status).technical_mode_enabled
    );
    renderAll();

    await pollStatus(true);
    await refreshLocalizedActiveTab();
  }

  async function refreshLocalizedActiveTab() {
    const status = getEntryPayload(state.data.status) || {};
    const ready = status.protocol_state === "ready" && status.init_ready && !status.busy;

    if (state.activeTab === "live" && ready) {
      await loadSensors(true);
      return;
    }

    if (state.activeTab === "dtc" && ready) {
      await loadErrors();
      return;
    }

    if (state.activeTab === "health") {
      await loadHealth(true);
      return;
    }

    if (state.activeTab === "technical" && state.technicalMode) {
      const technical = getEntryPayload(state.data.technical);
      if (technical && technical.sensors) {
        await loadCatalog();
        return;
      }

      if (technical && technical.packets && ready) {
        await loadMemory();
      }
    }
  }

  function messageTone(protocolState) {
    if (protocolState === "ready") return "ok";
    if (protocolState === "busy" || protocolState === "handshaking") return "warn";
    if (protocolState === "error") return "err";
    return "";
  }

  function isRebootPending() {
    return Date.now() < state.rebootPendingUntil;
  }

  function describeRequest(url) {
    if (url.startsWith("/api/status")) return t("sessionStatus");
    if (url.startsWith("/api/connect")) return t("connectEcu");
    if (url.startsWith("/api/reboot")) return t("rebootEsp32");
    if (url.startsWith("/api/sensors/collection")) return t("readLiveSensors");
    if (url.startsWith("/api/errors/clear")) return t("clearDtc");
    if (url.startsWith("/api/errors")) return t("readDtc");
    if (url.startsWith("/api/health")) return t("refreshHealth");
    if (url.startsWith("/api/memory/read")) return t("readMemory");
    if (url.startsWith("/api/sensors/catalog")) return t("loadSensorCatalog");
    if (url.startsWith("/api/technical-mode")) {
      return state.technicalMode ? t("technicalModeOff") : t("technicalModeOn");
    }
    return t("unknown");
  }

  function formatAge(timestamp) {
    if (!timestamp) {
      return t("ageNow");
    }

    const seconds = Math.max(0, Math.floor((Date.now() - timestamp) / 1000));
    if (seconds < 2) {
      return t("ageNow");
    }
    if (seconds < 60) {
      return t("ageSeconds", { count: seconds });
    }
    return t("ageMinutes", { count: Math.floor(seconds / 60) });
  }

  function formatElapsedMs(durationMs) {
    if (typeof durationMs !== "number" || !Number.isFinite(durationMs) || durationMs < 0) {
      return t("unknown");
    }

    const seconds = Math.floor(durationMs / 1000);
    if (seconds < 2) {
      return t("ageNow");
    }
    if (seconds < 60) {
      return t("ageSeconds", { count: seconds });
    }
    return t("ageMinutes", { count: Math.floor(seconds / 60) });
  }

  function escapeHtml(text) {
    return String(text)
      .replace(/&/g, "&amp;")
      .replace(/</g, "&lt;")
      .replace(/>/g, "&gt;")
      .replace(/"/g, "&quot;");
  }
})();
