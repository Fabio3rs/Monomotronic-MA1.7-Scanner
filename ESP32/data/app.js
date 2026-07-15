(function () {
  const translations = {
    en: {
      pageTitle: "MA1.7 ESP32 Scanner",
      topbarSubtitle: "Scanner portal",
      heroSubtitle: "Connect over the scanner access point, then use a single browser session for safe ECU reads.",
      onboardingHeading: "How to access the scanner portal",
      step1Title: "Join the scanner Wi-Fi",
      step1Copy: "Connect to the access point exposed by the ESP32 scanner.",
      step2Title: "Open the local page",
      step2Copy: "If the captive portal does not open by itself, browse to the local address shown here.",
      step3Title: "Connect the ECU",
      step3Copy: "Use the button below, then wait until the scanner reports that it is ready for reads.",
      primaryStatusHeading: "Scanner Status",
      statusCardCopy: "Use this area to verify the Wi-Fi link and ECU readiness before reading data.",
      connectionHeading: "Connect ECU",
      connectionLead: "Choose the vehicle profile first, then start the ECU session.",
      primaryWaiting: "Waiting for scanner status",
      primaryWaitingDetail: "Checking the ESP32 access point and the ECU link.",
      primaryPortalReady: "Scanner portal ready",
      primaryEcuDisconnected: "ECU disconnected",
      primaryHandshaking: "Handshake in progress",
      primaryReady: "Ready for reads",
      primaryBusy: "Scanner busy",
      primaryError: "Communication error",
      primaryOffline: "Scanner offline",
      primaryPortalDetail: "The ESP32 web portal is reachable. Connect the ECU when you are ready.",
      primaryDisconnectedDetail: "The Wi-Fi link is up, but the ECU session has not started yet.",
      primaryHandshakingDetail: "The scanner is initializing the ECU session. Wait for the ready state.",
      primaryReadyDetail: "The scanner can accept sensor, DTC, and health reads now.",
      primaryBusyDetail: "Another ECU operation is running. Wait for it to finish before sending a new one.",
      primaryErrorDetail: "The last ECU interaction ended in an error state. Refresh status and retry after checking wiring.",
      primaryOfflineDetail: "The browser cannot reach the ESP32 right now. Reconnect to the scanner Wi-Fi and refresh.",
      primaryRebooting: "ESP32 rebooting",
      primaryRebootingDetail: "Waiting for the scanner to come back after a reboot request.",
      scannerPortalReady: "Scanner portal ready",
      apInfo: "Wi-Fi: {ssid} | Local page: {url}",
      apInfoNoSsid: "Local page: {url}",
      networkHintOnline: "If the portal did not open automatically, use the local address shown above.",
      networkHintOffline: "If the page stops updating, reconnect to the scanner Wi-Fi and refresh.",
      languageLabel: "Language",
      profileLabel: "ECU profile",
      profileHint: "The selected profile defines the connection baud automatically.",
      profileBaud: "Baud: {baud}",
      profileBaudUnknown: "Baud: --",
      loadingProfiles: "Loading profiles...",
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
      waitingWifiReconnect: "Reconnect to the scanner Wi-Fi if this page does not update.",
      bootingStatus: "Starting scanner and Wi-Fi access point.",
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
      diagnosticsHeading: "Connection Diagnostics",
      diagnosticsCardCopy: "Use this section when the portal is reachable but the ECU session is not.",
      diagnosticsFooter: "Use this to separate Wi-Fi portal issues from ECU session issues.",
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
      metricProfile: "Profile",
      metricBaud: "Baud",
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
      connectionRestored: "Connection to the scanner was restored.",
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
      networkErrorRecovery: "Check that this phone is still connected to the scanner Wi-Fi, then refresh the page.",
      errorBusy: "The scanner is still handling another request. Wait for it to finish and try again.",
      errorAlreadyConnecting: "The ECU handshake is already in progress. Wait for the scanner to finish connecting.",
      errorAlreadyRunning: "The ECU worker is already running. Refresh status before trying another connect.",
      errorNotReady: "The ECU is not ready yet. Connect first or wait until the handshake completes.",
      errorTechnicalModeDisabled: "Enable technical mode inside the Technical tab before using this action.",
      errorInvalidRequest: "The request parameters are invalid for this endpoint.",
      errorReadFailed: "The read did not complete. Check ECU wiring and retry.",
      errorConnectFailed: "The scanner could not start the ECU session. Check ECU power, wiring, and ignition state.",
      errorCollectionFailed: "The live sensor read failed. Retry after the scanner returns to ready.",
      errorMemoryReadFailed: "The technical memory read failed. Recheck the address fields and retry.",
      requestFailed: "Request failed.",
      diagnosticsWaiting: "Waiting for scanner diagnostics.",
      diagnosticsAp: "AP",
      diagnosticsPortal: "Portal",
      diagnosticsSession: "ECU session",
      diagnosticsNetwork: "Frontend link",
      diagnosticsLastError: "Last error",
      diagnosticsLastPacket: "Last packet",
      diagnosticsSnapshot: "Snapshot",
      diagnosticsOnline: "online",
      diagnosticsOffline: "offline",
      diagnosticsAvailable: "available",
      diagnosticsUnavailable: "fallback",
      diagnosticsSessionDisconnected: "disconnected",
      diagnosticsSessionConnecting: "connecting",
      diagnosticsSessionReady: "ready",
      diagnosticsSessionBusy: "busy",
      diagnosticsSessionError: "error",
      diagnosticsSessionUnknown: "unknown",
      diagnosticsSnapshotLive: "live",
      diagnosticsSnapshotCached: "cached",
      diagnosticsSnapshotStale: "stale",
      diagnosticsSnapshotEmpty: "empty",
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
      topbarSubtitle: "Portal do scanner",
      heroSubtitle: "Conecte pelo ponto de acesso do scanner e use uma unica sessao web para leituras seguras da ECU.",
      onboardingHeading: "Como acessar o portal do scanner",
      step1Title: "Entre no Wi-Fi do scanner",
      step1Copy: "Conecte ao ponto de acesso exposto pelo ESP32 do scanner.",
      step2Title: "Abra a pagina local",
      step2Copy: "Se o captive portal nao abrir sozinho, navegue para o endereco local mostrado aqui.",
      step3Title: "Conecte a ECU",
      step3Copy: "Use o botao abaixo e aguarde o scanner informar que esta pronto para leituras.",
      primaryStatusHeading: "Status do scanner",
      statusCardCopy: "Use esta area para confirmar o link Wi-Fi e a prontidao da ECU antes de ler dados.",
      connectionHeading: "Conectar ECU",
      connectionLead: "Escolha primeiro o perfil do veiculo e depois inicie a sessao com a ECU.",
      primaryWaiting: "Aguardando status do scanner",
      primaryWaitingDetail: "Verificando o ponto de acesso do ESP32 e o link com a ECU.",
      primaryPortalReady: "Portal do scanner pronto",
      primaryEcuDisconnected: "ECU desconectada",
      primaryHandshaking: "Handshake em andamento",
      primaryReady: "Pronto para leituras",
      primaryBusy: "Scanner ocupado",
      primaryError: "Erro de comunicacao",
      primaryOffline: "Scanner offline",
      primaryPortalDetail: "O portal web do ESP32 esta acessivel. Conecte a ECU quando quiser iniciar a sessao.",
      primaryDisconnectedDetail: "O link Wi-Fi esta ativo, mas a sessao com a ECU ainda nao foi iniciada.",
      primaryHandshakingDetail: "O scanner esta inicializando a sessao com a ECU. Aguarde o estado pronto.",
      primaryReadyDetail: "O scanner pode aceitar leituras de sensores, DTC e saude agora.",
      primaryBusyDetail: "Outra operacao da ECU esta em andamento. Aguarde terminar antes de enviar uma nova.",
      primaryErrorDetail: "A ultima interacao com a ECU terminou em erro. Atualize o status e confira a fiacao.",
      primaryOfflineDetail: "O navegador nao consegue falar com o ESP32 agora. Reconecte ao Wi-Fi do scanner e atualize a pagina.",
      primaryRebooting: "ESP32 reiniciando",
      primaryRebootingDetail: "Aguardando o scanner voltar depois do pedido de reinicio.",
      scannerPortalReady: "Portal do scanner pronto",
      apInfo: "Wi-Fi: {ssid} | Pagina local: {url}",
      apInfoNoSsid: "Pagina local: {url}",
      networkHintOnline: "Se o portal nao abriu automaticamente, use o endereco local mostrado acima.",
      networkHintOffline: "Se a pagina parar de atualizar, reconecte ao Wi-Fi do scanner e atualize.",
      languageLabel: "Idioma",
      profileLabel: "Perfil da ECU",
      profileHint: "O perfil selecionado define automaticamente o baud da conexao.",
      profileBaud: "Baud: {baud}",
      profileBaudUnknown: "Baud: --",
      loadingProfiles: "Carregando perfis...",
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
      waitingWifiReconnect: "Reconecte ao Wi-Fi do scanner se esta pagina nao atualizar.",
      bootingStatus: "Iniciando scanner e ponto de acesso Wi-Fi.",
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
      diagnosticsHeading: "Diagnostico de conexao",
      diagnosticsCardCopy: "Use esta secao quando o portal estiver acessivel, mas a sessao com a ECU nao estiver.",
      diagnosticsFooter: "Use isto para separar problemas do portal Wi-Fi de problemas da sessao com a ECU.",
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
      metricProfile: "Perfil",
      metricBaud: "Baud",
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
      connectionRestored: "A conexao com o scanner foi restabelecida.",
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
      networkErrorRecovery: "Confira se este celular ainda esta conectado ao Wi-Fi do scanner e atualize a pagina.",
      errorBusy: "O scanner ainda esta processando outra requisicao. Aguarde terminar e tente de novo.",
      errorAlreadyConnecting: "O handshake com a ECU ja esta em andamento. Aguarde o scanner concluir a conexao.",
      errorAlreadyRunning: "A thread da ECU ja esta em execucao. Atualize o status antes de tentar conectar de novo.",
      errorNotReady: "A ECU ainda nao esta pronta. Conecte primeiro ou aguarde o fim do handshake.",
      errorTechnicalModeDisabled: "Ative o modo tecnico dentro da aba Tecnico antes de usar esta acao.",
      errorInvalidRequest: "Os parametros enviados sao invalidos para este endpoint.",
      errorReadFailed: "A leitura nao terminou corretamente. Confira a fiacao da ECU e tente novamente.",
      errorConnectFailed: "O scanner nao conseguiu iniciar a sessao com a ECU. Confira alimentacao, fiacao e ignicao.",
      errorCollectionFailed: "A leitura de sensores ao vivo falhou. Tente de novo quando o scanner voltar ao estado pronto.",
      errorMemoryReadFailed: "A leitura tecnica de memoria falhou. Revise os campos de endereco e tente novamente.",
      requestFailed: "Falha na requisicao.",
      diagnosticsWaiting: "Aguardando diagnostico do scanner.",
      diagnosticsAp: "AP",
      diagnosticsPortal: "Portal",
      diagnosticsSession: "Sessao ECU",
      diagnosticsNetwork: "Link do navegador",
      diagnosticsLastError: "Ultimo erro",
      diagnosticsLastPacket: "Ultimo pacote",
      diagnosticsSnapshot: "Snapshot",
      diagnosticsOnline: "online",
      diagnosticsOffline: "offline",
      diagnosticsAvailable: "disponivel",
      diagnosticsUnavailable: "fallback",
      diagnosticsSessionDisconnected: "desconectada",
      diagnosticsSessionConnecting: "conectando",
      diagnosticsSessionReady: "pronta",
      diagnosticsSessionBusy: "ocupada",
      diagnosticsSessionError: "erro",
      diagnosticsSessionUnknown: "desconhecida",
      diagnosticsSnapshotLive: "ao vivo",
      diagnosticsSnapshotCached: "cache",
      diagnosticsSnapshotStale: "desatualizado",
      diagnosticsSnapshotEmpty: "vazio",
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
    prefProfile: "ma17.pref.profile",
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
  const initialProfileId = localStorage.getItem(storageKeys.prefProfile) || "";
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
    selectedProfileId: initialProfileId,
    backendSelectedProfileId: "",
    defaultProfileId: "",
    profiles: [],
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
    pendingConnect: false,
    hasReceivedStatus: false,
    lastErrorCode: "",
    lastErrorMessage: "",
    data: {
      status: initialStatusEntry,
      live: initialLiveEntry,
      dtc: initialDtcEntry,
      health: initialHealthEntry,
      technical: chooseMostRecentEntry(initialTechnicalSessionEntry, initialCatalogEntry)
    }
  };

  const bootConfig = normalizeBootConfig(window.__MA17_BOOT__ || {});

  const els = {
    topbarTitle: byId("topbarTitle"),
    topbarSubtitle: byId("topbarSubtitle"),
    title: document.querySelector(".title"),
    subtitle: document.querySelector(".subtitle"),
    primaryBadge: byId("primaryBadge"),
    liveBadge: byId("liveBadge"),
    netBadge: byId("netBadge"),
    cacheBadge: byId("cacheBadge"),
    apInfo: byId("apInfo"),
    onboardingHeading: byId("onboardingHeading"),
    step1Title: byId("step1Title"),
    step1Copy: byId("step1Copy"),
    step2Title: byId("step2Title"),
    step2Copy: byId("step2Copy"),
    step3Title: byId("step3Title"),
    step3Copy: byId("step3Copy"),
    primaryStatusHeading: byId("primaryStatusHeading"),
    statusCardCopy: byId("statusCardCopy"),
    connectionHeading: byId("connectionHeading"),
    connectionLead: byId("connectionLead"),
    primaryStatus: byId("primaryStatus"),
    primaryStatusDetail: byId("primaryStatusDetail"),
    networkHint: byId("networkHint"),
    pollInfo: byId("pollInfo"),
    profileBaudChip: byId("profileBaudChip"),
    profileHint: byId("profileHint"),
    messageBox: byId("messageBox"),
    diagnosticsSummary: byId("diagnosticsSummary"),
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
    profileLabel: byId("profileLabel"),
    profileSelect: byId("profileSelect"),
    sessionStatusHeading: byId("sessionStatusHeading"),
    tabLiveButton: document.querySelector('.tab[data-tab="live"]'),
    tabDtcButton: document.querySelector('.tab[data-tab="dtc"]'),
    tabHealthButton: document.querySelector('.tab[data-tab="health"]'),
    tabTechnicalButton: document.querySelector('.tab[data-tab="technical"]'),
    tabButtons: Array.from(document.querySelectorAll(".tab")),
    collectionTableLabel: byId("collectionTableLabel"),
    tableAutoOption: document.querySelector('#tableSelect option[value="0"]'),
    table1Option: document.querySelector('#tableSelect option[value="1"]'),
    table2Option: document.querySelector('#tableSelect option[value="2"]'),
    sensorTableHead: document.querySelector(".table-head"),
    technicalSummary: byId("technicalSummary"),
    memHiLabel: byId("memHiLabel"),
    memLoLabel: byId("memLoLabel"),
    memLenLabel: byId("memLenLabel"),
    currentProtocolHeading: byId("currentProtocolHeading"),
    currentProtocolFooter: byId("currentProtocolFooter"),
    cachedSnapshotHeading: byId("cachedSnapshotHeading"),
    cachedSnapshotFooter: byId("cachedSnapshotFooter"),
    diagnosticsHeading: byId("diagnosticsHeading"),
    diagnosticsCardCopy: byId("diagnosticsCardCopy"),
    diagnosticsFooter: byId("diagnosticsFooter"),
    tableSelect: byId("tableSelect"),
    memHi: byId("memHi"),
    memLo: byId("memLo"),
    memLen: byId("memLen")
  };

  bindEvents();
  initialize();

  function byId(id) {
    return document.getElementById(id);
  }

  function normalizeBootConfig(raw) {
    const cfg = raw && typeof raw === "object" ? raw : {};
    const apUrl = typeof cfg.apUrl === "string" && cfg.apUrl
      ? cfg.apUrl
      : `${window.location.protocol}//${window.location.host}/`;
    const apIp = typeof cfg.apIp === "string" && cfg.apIp
      ? cfg.apIp
      : window.location.hostname;
    return {
      apSsid: typeof cfg.apSsid === "string" ? cfg.apSsid : "",
      apIp,
      apUrl,
      webUiAvailable: cfg.webUiAvailable !== false
    };
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
    els.topbarTitle.textContent = t("pageTitle");
    els.topbarSubtitle.textContent = t("topbarSubtitle");
    els.title.textContent = t("pageTitle");
    els.subtitle.textContent = t("heroSubtitle");
    els.onboardingHeading.textContent = t("onboardingHeading");
    els.step1Title.textContent = t("step1Title");
    els.step1Copy.textContent = t("step1Copy");
    els.step2Title.textContent = t("step2Title");
    els.step2Copy.textContent = t("step2Copy");
    els.step3Title.textContent = t("step3Title");
    els.step3Copy.textContent = t("step3Copy");
    els.primaryStatusHeading.textContent = t("primaryStatusHeading");
    els.statusCardCopy.textContent = t("statusCardCopy");
    els.connectionHeading.textContent = t("connectionHeading");
    els.connectionLead.textContent = t("connectionLead");
    els.languageLabel.textContent = t("languageLabel");
    els.profileLabel.textContent = t("profileLabel");
    els.profileHint.textContent = t("profileHint");
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
    els.diagnosticsHeading.textContent = t("diagnosticsHeading");
    els.diagnosticsCardCopy.textContent = t("diagnosticsCardCopy");
    els.diagnosticsFooter.textContent = t("diagnosticsFooter");
    els.apInfo.textContent = buildApInfoText();
    renderProfileOptions();
  }

  function bindEvents() {
    document.querySelectorAll(".tab").forEach((button) => {
      button.addEventListener("click", () => setTab(button.dataset.tab));
    });

    els.languageSelect.addEventListener("change", onLocaleChange);
    els.profileSelect.addEventListener("change", onProfileChange);
    els.tableSelect.addEventListener("change", onCollectionTableChange);

    els.connectBtn.addEventListener("click", () => {
      const profileId = getPreferredProfileId();
      const connectUrl = profileId
        ? `/api/connect?profile=${encodeURIComponent(profileId)}`
        : "/api/connect";
      return runAction(connectUrl, { method: "POST" }, onStatusResponse);
    });
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

  async function initialize() {
    renderAll();
    await loadProfiles();
    await pollStatus(true);
  }

  function normalizeProfiles(profiles) {
    if (!Array.isArray(profiles)) {
      return [];
    }

    return profiles.filter((profile) =>
      profile &&
      typeof profile.id === "string" &&
      profile.id &&
      typeof profile.label === "string" &&
      Number.isFinite(Number(profile.session_baud))
    ).map((profile) => ({
      id: profile.id,
      label: profile.label,
      session_baud: Number(profile.session_baud)
    }));
  }

  function findProfileById(profileId) {
    if (!profileId) {
      return null;
    }
    return state.profiles.find((profile) => profile.id === profileId) || null;
  }

  function getPreferredProfileId() {
    if (findProfileById(state.selectedProfileId)) {
      return state.selectedProfileId;
    }
    if (findProfileById(state.backendSelectedProfileId)) {
      return state.backendSelectedProfileId;
    }
    if (findProfileById(state.defaultProfileId)) {
      return state.defaultProfileId;
    }
    return state.profiles.length ? state.profiles[0].id : "";
  }

  function getDisplayedProfileId() {
    if (findProfileById(state.backendSelectedProfileId)) {
      return state.backendSelectedProfileId;
    }
    return getPreferredProfileId();
  }

  function setSelectedProfileId(profileId, persist) {
    state.selectedProfileId = profileId || "";
    if (!persist) {
      return;
    }

    try {
      if (state.selectedProfileId) {
        localStorage.setItem(storageKeys.prefProfile, state.selectedProfileId);
      } else {
        localStorage.removeItem(storageKeys.prefProfile);
      }
    } catch (_) {
      // Ignore storage failures.
    }
  }

  function renderProfileOptions() {
    if (!els.profileSelect) {
      return;
    }

    if (!state.profiles.length) {
      els.profileSelect.innerHTML =
        `<option value="">${escapeHtml(t("loadingProfiles"))}</option>`;
      els.profileSelect.value = "";
      return;
    }

    const selectedId = getDisplayedProfileId();
    els.profileSelect.innerHTML = state.profiles.map((profile) => `
      <option value="${escapeHtml(profile.id)}">${escapeHtml(profile.label)}</option>
    `).join("");
    els.profileSelect.value = selectedId;
  }

  async function loadProfiles(silent) {
    try {
      const response = await fetch(buildApiUrl("/api/config/profiles"));
      const payload = await response.json();
      if (!response.ok || payload.ok === false) {
        if (!silent) {
          handleErrorPayload(payload);
        }
        return;
      }

      state.profiles = normalizeProfiles(payload.profiles);
      state.defaultProfileId =
        typeof payload.default_profile_id === "string" ? payload.default_profile_id : "";
      state.backendSelectedProfileId =
        typeof payload.selected_profile_id === "string" ? payload.selected_profile_id : "";
      setSelectedProfileId(getPreferredProfileId(), true);
      renderAll();
    } catch (error) {
      if (!silent) {
        setMessage(`${t("networkError", { message: error.message })}\n${t("networkErrorRecovery")}`, "err");
      }
    }
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

  function onProfileChange() {
    const nextProfileId = els.profileSelect.value;
    if (!findProfileById(nextProfileId)) {
      renderProfileOptions();
      return;
    }

    setSelectedProfileId(nextProfileId, true);
    renderAll();
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
    renderPrimaryStatus();
    renderProfileSummary();
    renderNetworkBadge();
    renderCacheSummary();
    renderDiagnostics();
    renderTechnicalMode();
    renderCachedSections();
    renderSectionMeta();
    syncControls();
  }

  function describeProfile(profileId, sessionBaud) {
    const profile = findProfileById(profileId);
    if (profile) {
      return profile.label;
    }
    if (profileId && sessionBaud) {
      return `${profileId} (${sessionBaud})`;
    }
    if (sessionBaud) {
      return String(sessionBaud);
    }
    return t("unknown");
  }

  function renderProfileSummary() {
    const status = getEntryPayload(state.data.status) || {};
    const selectedProfile = findProfileById(getDisplayedProfileId());
    const baud = status.session_baud || (selectedProfile ? selectedProfile.session_baud : 0);
    els.profileBaudChip.textContent = baud
      ? t("profileBaud", { baud: String(baud) })
      : t("profileBaudUnknown");
  }

  function renderStatus() {
    const status = getEntryPayload(state.data.status);
    if (!status) {
      els.statusGrid.innerHTML = "";
      els.primaryBadge.textContent = getPrimaryStatusTitle(status);
      els.liveBadge.textContent = t("noLiveData");
      els.pollInfo.textContent = describePolling(status);
      els.stateSummary.textContent = t("noStatusResponse");
      if (!state.requestInFlight) {
        setMessage(describeDefaultMessage(), messageToneForUiState(deriveUiState()));
      }
      return;
    }

    const metrics = [
      [t("metricProtocol"), translateProtocolState(status.protocol_state || "unknown")],
      [t("metricOperation"), translateOperation(status.current_operation || "none")],
      [t("metricConnected"), status.connected ? t("yes") : t("no")],
      [t("metricReady"), status.init_ready ? t("yes") : t("no")],
      [t("metricBusy"), status.busy ? t("yes") : t("no")],
      [t("metricProfile"), describeProfile(status.selected_profile_id, status.session_baud)],
      [t("metricBaud"), status.session_baud ? String(status.session_baud) : t("unknown")],
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
    els.primaryBadge.textContent = getPrimaryStatusTitle(status);

    els.pollInfo.textContent = describePolling(status);
  }

  function renderPrimaryStatus() {
    const status = getEntryPayload(state.data.status);
    els.primaryStatus.textContent = getPrimaryStatusTitle(status);
    els.primaryStatusDetail.textContent = getPrimaryStatusDetail(status);
    els.networkHint.textContent = state.backendReachable
      ? t("networkHintOnline")
      : t("networkHintOffline");
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

  function deriveUiState() {
    const status = getEntryPayload(state.data.status);
    const ready = status && status.meta && typeof status.meta.ready_for_commands === "boolean"
      ? status.meta.ready_for_commands
      : Boolean(status && status.protocol_state === "ready" && status.init_ready && !status.busy);

    if (isRebootPending()) {
      return "rebooting";
    }
    if (!state.hasReceivedStatus && !state.backendReachable) {
      return "booting";
    }
    if (!state.backendReachable) {
      return "network_lost";
    }
    if (state.pendingConnect || (status && status.protocol_state === "handshaking")) {
      return "connecting_ecu";
    }
    if (status && status.protocol_state === "error") {
      return "ecu_error";
    }
    if (status && (status.protocol_state === "busy" || status.busy)) {
      return "ecu_busy";
    }
    if (ready) {
      return "ecu_ready";
    }
    if (!status || !status.protocol_state) {
      return "booting";
    }
    return "portal_ready";
  }

  function translateDiagnosticsSession(uiState) {
    if (uiState === "connecting_ecu") return t("diagnosticsSessionConnecting");
    if (uiState === "ecu_ready") return t("diagnosticsSessionReady");
    if (uiState === "ecu_busy") return t("diagnosticsSessionBusy");
    if (uiState === "ecu_error") return t("diagnosticsSessionError");
    if (uiState === "portal_ready") return t("diagnosticsSessionDisconnected");
    if (uiState === "network_lost") return `${t("diagnosticsSessionDisconnected")} (${t("diagnosticsSnapshotStale")})`;
    return t("diagnosticsSessionUnknown");
  }

  function translateSnapshotLabel() {
    const primaryEntry = state.data.live || state.data.status;
    if (!primaryEntry) return t("diagnosticsSnapshotEmpty");
    if (isEntryExpired(primaryEntry)) return t("diagnosticsSnapshotStale");
    if (primaryEntry.source === "live") return t("diagnosticsSnapshotLive");
    return t("diagnosticsSnapshotCached");
  }

  function describeSnapshotState() {
    return `${t("diagnosticsSnapshot")}: ${translateSnapshotLabel()}.`;
  }

  function renderDiagnostics() {
    const status = getEntryPayload(state.data.status);
    if (!status && !bootConfig.apSsid && !bootConfig.apIp) {
      els.diagnosticsSummary.textContent = t("diagnosticsWaiting");
      return;
    }

    const meta = status && status.meta ? status.meta : null;
    const apActive = Boolean(meta ? meta.ap_active : bootConfig.apIp);
    const webUiAvailable = meta && typeof meta.web_ui_available === "boolean"
      ? meta.web_ui_available
      : bootConfig.webUiAvailable;
    const lines = [
      `${t("diagnosticsAp")}: ${apActive ? t("diagnosticsOnline") : t("diagnosticsOffline")} (${buildApInfoText()})`,
      `${t("diagnosticsPortal")}: ${webUiAvailable ? t("diagnosticsAvailable") : t("diagnosticsUnavailable")}`,
      `${t("diagnosticsNetwork")}: ${state.backendReachable ? t("diagnosticsOnline") : t("diagnosticsOffline")}`,
      `${t("diagnosticsSession")}: ${translateDiagnosticsSession(deriveUiState())}`,
      `${t("diagnosticsSnapshot")}: ${translateSnapshotLabel()}`
    ];

    if (state.lastErrorCode) {
      lines.push(`${t("diagnosticsLastError")}: ${state.lastErrorCode}`);
    }

    if (meta && typeof meta.last_packet_age_ms === "number") {
      lines.push(`${t("diagnosticsLastPacket")}: ${formatElapsedMs(meta.last_packet_age_ms)}`);
    }

    els.diagnosticsSummary.textContent = lines.join("\n");
  }

  function buildApInfoText() {
    const status = getEntryPayload(state.data.status);
    const meta = status && status.meta ? status.meta : null;
    const apSsid = meta && meta.ap_ssid ? meta.ap_ssid : bootConfig.apSsid;
    const apIp = meta && meta.ap_ip ? meta.ap_ip : bootConfig.apIp;
    const apUrl = apIp ? `http://${apIp}/` : bootConfig.apUrl;
    if (apSsid) {
      return t("apInfo", { ssid: apSsid, url: apUrl });
    }
    return t("apInfoNoSsid", { url: apUrl });
  }

  function getPrimaryStatusTitle(status) {
    const uiState = deriveUiState();
    if (uiState === "rebooting") return t("primaryRebooting");
    if (uiState === "network_lost") return t("primaryOffline");
    if (uiState === "booting") return t("primaryWaiting");
    if (uiState === "connecting_ecu") return t("primaryHandshaking");
    if (uiState === "ecu_ready") return t("primaryReady");
    if (uiState === "ecu_busy") return t("primaryBusy");
    if (uiState === "ecu_error") return t("primaryError");
    return t("primaryEcuDisconnected");
  }

  function getPrimaryStatusDetail(status) {
    const uiState = deriveUiState();
    if (uiState === "rebooting") return t("primaryRebootingDetail");
    if (uiState === "network_lost") return `${t("primaryOfflineDetail")} ${describeSnapshotState()}`;
    if (uiState === "booting") return t("primaryWaitingDetail");
    if (uiState === "connecting_ecu") return t("primaryHandshakingDetail");
    if (uiState === "ecu_ready") return t("primaryReadyDetail");
    if (uiState === "ecu_busy") return t("primaryBusyDetail");
    if (uiState === "ecu_error") return t("primaryErrorDetail");
    return t("primaryDisconnectedDetail");
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
    const uiState = deriveUiState();
    const ready = Boolean(
      status.meta && typeof status.meta.ready_for_commands === "boolean"
        ? status.meta.ready_for_commands
        : status.protocol_state === "ready" && status.init_ready && !status.busy
    );
    const technicalEnabled = state.technicalMode;
    const rebootPending = isRebootPending();
    const connectLocked = state.requestInFlight ||
      rebootPending ||
      uiState === "connecting_ecu" ||
      uiState === "ecu_busy" ||
      uiState === "ecu_ready";
    const profileLocked = connectLocked;

    els.connectBtn.disabled = connectLocked || uiState === "network_lost";
    els.rebootBtn.disabled = state.requestInFlight || rebootPending;
    els.refreshBtn.disabled = state.requestInFlight || rebootPending;
    els.technicalToggleBtn.disabled = state.requestInFlight || rebootPending;
    els.languageSelect.disabled = state.requestInFlight || rebootPending;
    els.profileSelect.disabled = profileLocked || !state.profiles.length;
    els.loadSensorsBtn.disabled = state.requestInFlight || rebootPending || !ready || uiState === "network_lost";
    els.readErrorsBtn.disabled = state.requestInFlight || rebootPending || !ready || uiState === "network_lost";
    els.clearErrorsBtn.disabled = state.requestInFlight || rebootPending || !ready || uiState === "network_lost";
    els.loadHealthBtn.disabled = state.requestInFlight || rebootPending || uiState === "network_lost";
    els.readMemoryBtn.disabled = state.requestInFlight || rebootPending || !ready || !technicalEnabled || uiState === "network_lost";
    els.loadCatalogBtn.disabled = state.requestInFlight || rebootPending || !technicalEnabled || uiState === "network_lost";
  }

  async function pollStatus(forceMessage) {
    await runAction("/api/status", {}, (payload) => {
      onStatusResponse(payload);
      if (forceMessage) {
        setMessage(describeDefaultMessage(), messageToneForUiState(deriveUiState()));
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
    state.pendingConnect = false;
    state.hasReceivedStatus = true;
    if (typeof payload.selected_profile_id === "string") {
      state.backendSelectedProfileId = payload.selected_profile_id;
    }
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
    state.pendingConnect = false;
    renderAll();
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
    if (url.startsWith("/api/connect")) {
      state.pendingConnect = true;
    }
    state.lastRequestStartedAt = Date.now();
    state.activeRequestLabel = describeRequest(url);
    renderAll();
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
        state.hasReceivedStatus = true;
        handleErrorPayload(payload);
        return null;
      }

      const recovered = !state.backendReachable;
      state.backendReachable = true;
      state.lastRoundTripMs = elapsedMs;
      state.lastSuccessAt = Date.now();

      if (payload.protocol_state) {
        onStatusResponse(payload);
      }

      onSuccess(payload);
      if (recovered) {
        setMessage(t("connectionRestored"), "ok");
      } else if (cfg.successMessageKey) {
        setMessage(t(cfg.successMessageKey), "ok");
      } else if (cfg.showFinishedMessage) {
        setMessage(t("requestFinished", { operation: state.activeRequestLabel, ms: elapsedMs }), "ok");
      }
      renderAll();
      return payload;
    } catch (error) {
      state.backendReachable = false;
      state.pendingConnect = false;
      renderAll();
      if (!cfg.silentNetworkError) {
        setMessage(`${t("networkError", { message: error.message })}\n${t("networkErrorRecovery")}`, "err");
      }
      return null;
    } finally {
      state.requestInFlight = false;
      state.activeRequestLabel = "";
      renderAll();
      schedulePolling();
    }
  }

  function handleErrorPayload(payload) {
    if (payload && payload.protocol_state) {
      state.pendingConnect = payload.protocol_state === "handshaking";
      setDataEntry("status", payload, {
        storageKey: getLocalizedStorageKey(storageKeys.cacheStatus),
        ttlMs: dataTtls.status,
        source: "live"
      });
      state.hasReceivedStatus = true;
    }

    const code = payload && payload.error_code ? payload.error_code : "request_failed";
    state.lastErrorCode = code;
    const message = formatBackendError(payload);
    state.lastErrorMessage = message;
    renderAll();
    setMessage(`${code}: ${message}`, "err");
  }

  function formatBackendError(payload) {
    const code = payload && payload.error_code ? payload.error_code : "";
    const message = payload && payload.message ? payload.message : t("requestFailed");
    if (code === "busy") {
      return t("errorBusy");
    }
    if (code === "already_connecting") {
      return t("errorAlreadyConnecting");
    }
    if (code === "already_running") {
      return t("errorAlreadyRunning");
    }
    if (code === "not_ready") {
      return t("errorNotReady");
    }
    if (code === "technical_mode_disabled") {
      return t("errorTechnicalModeDisabled");
    }
    if (code === "invalid_request") {
      return t("errorInvalidRequest");
    }
    if (code === "read_failed" || code === "clear_failed") {
      return t("errorReadFailed");
    }
    if (code === "connect_failed") {
      return t("errorConnectFailed");
    }
    if (code === "collection_failed") {
      return t("errorCollectionFailed");
    }
    if (code === "memory_read_failed") {
      return t("errorMemoryReadFailed");
    }
    return message;
  }

  function setMessage(text, tone) {
    els.messageBox.className = `message ${tone || ""}`.trim();
    els.messageBox.textContent = text;
  }

  function describePolling(status) {
    const uiState = deriveUiState();
    if (isRebootPending()) {
      return t("rebootPendingPoll");
    }
    if (uiState === "network_lost") {
      return t("waitingWifiReconnect");
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

    await loadProfiles(true);
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

  function messageToneForUiState(uiState) {
    if (uiState === "ecu_ready") return "ok";
    if (uiState === "connecting_ecu" || uiState === "ecu_busy" || uiState === "rebooting") return "warn";
    if (uiState === "network_lost" || uiState === "ecu_error") return "err";
    return "";
  }

  function describeDefaultMessage() {
    const uiState = deriveUiState();
    if (uiState === "booting") {
      return t("bootingStatus");
    }
    if (uiState === "portal_ready") {
      return t("primaryPortalDetail");
    }
    if (uiState === "connecting_ecu") {
      return t("primaryHandshakingDetail");
    }
    if (uiState === "ecu_ready") {
      return t("primaryReadyDetail");
    }
    if (uiState === "ecu_busy") {
      return t("primaryBusyDetail");
    }
    if (uiState === "ecu_error") {
      return t("primaryErrorDetail");
    }
    if (uiState === "rebooting") {
      return t("rebootingEsp32");
    }
    return `${t("networkError", { message: "ESP32 unreachable" })}\n${t("networkErrorRecovery")}`;
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
