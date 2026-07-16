# ESP32 Session Handoff

Data da sessão: 2026-06-29

## Objetivo desta rodada

Avançar a versão `ESP32/` priorizando paridade funcional com a versão PC, sem ainda fazer a reorganização grande de arquitetura compartilhada.

## O que foi feito

### 1. Consolidação do firmware principal

- `ESP32/MMScanner.ino` passou a ser o firmware principal.
- `ESP32/ESP32Scanner.ino` foi reduzido a placeholder legado sem `setup()`/`loop()` para não competir no build Arduino.

### 2. Refactor do núcleo `ESP32Monomotronic`

Arquivos principais:

- `ESP32/ESP32Monomotronic.h`
- `ESP32/ESP32Monomotronic.cpp`

Mudanças introduzidas:

- adição de `ESP32ScannerConfig` com:
  - `session_baud`
  - `init_address`
  - pinos do init
  - timeouts básicos
  - intervalo de keep-alive
- troca de pacote dinâmico por estrutura com buffer fixo:
  - `ECUmmpacket::data` virou `std::array<uint8_t, 252>`
  - uso de `data_length`
- criação de `ECUResponseCollection` com array fixo
- adição de `ECUHealthSnapshot`
- inclusão das operações:
  - `readECUMemory()`
  - `requestSensorCollection()`
  - `determineCollectionTable()`
- inclusão dos frame IDs:
  - `ECU_REQ_SENSOR_COLLECTION = 0x12`
  - `ECU_RESP_SENSOR_COLLECTION = 0xF4`
- unificação do `optional` local em `ESP32/optional.h`

### 3. Camada de catálogo/tabelas para F4

Arquivos novos:

- `ESP32/SensorCatalog.h`
- `ESP32/SensorCatalog.cpp`

Conteúdo:

- tabelas de collection 1 e 2
- catálogo mínimo de sensores
- decoders float básicos para leituras mais relevantes

Observação importante:

- esses arquivos ainda estavam `untracked` no `git status` no final da sessão

### 4. API HTTP e console serial

Arquivo principal:

- `ESP32/MMScanner.ino`

Foi adicionada API JSON para:

- `/api/status`
- `/api/health`
- `/api/errors`
- `/api/errors/clear`
- `/api/memory/read`
- `/api/sensors/collection`
- `/api/sensors/catalog`

Também foram adicionados comandos via serial:

- `help`
- `status`
- `connect`
- `health`
- `errors`
- `clear`
- `catalog`
- `f4 [table]`
- `mem <hi> <lo> <len>`

### 5. Remoção do logger ocioso

O logger antigo foi removido porque não havia nenhum writer ativo.

Arquivos removidos:

- `ESP32/Log.h`
- `ESP32/Log.cpp`

Também foram removidos:

- endpoint `/log`
- endpoint `/api/log`
- comando serial `log`

## Correções pontuais feitas durante a sessão

### `errorPacketToString`

Foi restaurado o conteúdo textual original da lista de erros no trecho:

- `ESP32/ESP32Monomotronic.cpp`

Incluindo:

- comentário `Source: http://www.fiat-tipo.ru/fpost8823.html`
- descrições originais
- bloco de comentário sobre `1E` / `9E`

Diferença mantida de propósito:

- `p.data_length >= 3` no lugar de `p.data.size() >= 3`

Isso foi necessário porque a estrutura do pacote deixou de usar `std::vector`.

## Problemas e riscos ainda em aberto

### 1. Handshake / baixo nível do protocolo

Pontos a revisar com cuidado:

- `baudInit()`
- `ECUWrite()`
- `ECUWriteWaitResponse()`

Essas partes sofreram mudanças comportamentais e precisam de validação real em hardware.

### 2. Concorrência em `initPackets_`

Ainda existe preocupação com:

- `getInitPackets()` retornando ponteiro cru
- acesso web lendo enquanto a task de comunicação pode limpar/repopular esse buffer

Esse ponto ficou auditado, mas não foi corrigido ainda.

### 3. Design de logging futuro

Foi decidido remover o logger atual em vez de evoluí-lo.

Direção desejada para o futuro:

- nenhum uso de string no hot path do core do protocolo
- eventual logger/event buffer estruturado e fixo

### 4. Arquivos novos fora do índice

Conferir antes da próxima sessão:

- `ESP32/SensorCatalog.h`
- `ESP32/SensorCatalog.cpp`

## Estado final observado no `ESP32/`

Arquivos modificados principais:

- `ESP32/ESP32Monomotronic.h`
- `ESP32/ESP32Monomotronic.cpp`
- `ESP32/MMScanner.ino`
- `ESP32/ESP32Scanner.ino`
- `ESP32/optional.h`

Arquivos removidos:

- `ESP32/Log.h`
- `ESP32/Log.cpp`

Arquivos adicionados:

- `ESP32/SensorCatalog.h`
- `ESP32/SensorCatalog.cpp`

## Próximos passos recomendados

1. Validar e possivelmente restaurar/ajustar o comportamento de:
   - `baudInit()`
   - `ECUWrite()`
   - `ECUWriteWaitResponse()`
2. Corrigir o acesso concorrente a `initPackets_`
3. Confirmar `SensorCatalog.*` no índice do git
4. Testar em hardware real:
   - handshake
   - init string
   - `ECUReadErrors()`
   - `readECUMemory()`
   - `requestSensorCollection()`
5. Só depois retomar discussão de logger embarcado/event buffer sem strings no hot path

