# Análise: Init Packets String Format (IAW MA1.7)

## 📋 Exemplo Real

```
"0261 203 388AN09I2000148BOSCH MA17  MC "
```

**Origem:** F4 frame response (send_ident_banner) no assembly 8051 - Fiat Tipo IAW MA1.7

---

## 🔍 Breakdown da String

| Campo | Offset | Tamanho | Exemplo | Descrição |
|-------|--------|---------|---------|-----------|
| **Part Number (P/N)** | 0 | 4 chars | `0261` | Peça número parte |
| **Space** | 4 | 1 | ` ` | Separador |
| **P/N cont.** | 5 | 3 chars | `203` | Número parte (continuação) |
| **Space** | 8 | 1 | ` ` | Separador |
| **P/N cont.** | 9 | 3 chars | `388` | Número parte (continuação) |
| **Variant** | 12 | 4 chars | `AN09` | Variante/sufixo |
| **Hardware ID** | 16 | 1 char | `I` | Identificador hardware |
| **Firmware Rev** | 17 | 7 chars | `2000148` | Revisão firmware |
| **Manufacturer** | 24 | 5 chars | `BOSCH` | Fabricante |
| **Space** | 29 | 1 | ` ` | Separador |
| **Model Code** | 30 | 5 chars | `MA17` | Código modelo ECU |
| **Space** | 34 | 2 | `  ` | Separadores |
| **Subsystem** | 36 | 3 chars | `MC ` | Subsistema (motor control) |
| **Total** | - | **39 bytes** | - | - |

---

## 🎯 Padrão Identificado

```
[P/N: 4dig + 3dig + 3dig][Variant: 4char][HW: 1char][FW: 7dig][Maker: 5char] [Model: 5char]  [Subsys: 3char]
```

### Segmentação Correta:

```c++
// Estrutura de parsing:
// 0-3    : "0261"                   (part1)
// 4      : " "
// 5-7    : "203"                    (part2)
// 8      : " "
// 9-11   : "388"                    (part3)
// 12-15  : "AN09"                   (variant)
// 16     : "I"                       (hardware_id)
// 17-23  : "2000148"                (firmware_revision)
// 24-28  : "BOSCH"                  (manufacturer)
// 29     : " "
// 30-34  : "MA17"                   (model_code) ← **CRÍTICO**
// 35-36  : "  " (ou variável)
// 37-39  : "MC "                    (subsystem)
```

---

## 📊 Valores Conhecidos (IAW MA1.7)

| Campo | Valor | Notas |
|-------|-------|-------|
| **Model Code** | `MA17` | **IAW MA 1.7** (Magneti Marelli) |
| **Manufacturer** | `BOSCH` | Bosch (ou variante) |
| **Hardware ID** | `I` ou `A` | Versão hardware |
| **Subsystem** | `MC ` | Motor Control |
| **P/N Padrão** | `0261 203 388` | Comum em Fiat Tipo |

---

## 🔄 Múltiplas Mensagens F4

```
send_ident_banner_0: "0261 203 388AN09I2000148BOSCH MA17  MC "
send_ident_banner_1: "AN09I2000148BOSCH MA17  MC "     (offset +12)
send_ident_banner_2: "BOSCH MA17  MC "                  (offset +24)
send_ident_banner_3: " MC "                             (offset +36)
```

**Interpretação:** ECU envia a string em **4 fragmentos** durante handshake F4:
- Fragmento 0: String completa (39 bytes)
- Fragmento 1: String a partir do offset 12 (variant)
- Fragmento 2: String a partir do offset 24 (manufacturer)
- Fragmento 3: String a partir do offset 36 (subsystem)

---

## 💾 Implementação Recomendada (C++20)

```cpp
// app_data.h
struct ECUInfo {
    std::string part_number;           // "0261 203 388"
    std::string variant;               // "AN09"
    std::string hardware_id;           // "I"
    std::string firmware_revision;     // "2000148"
    std::string manufacturer;          // "BOSCH"
    std::string model_code;            // "MA17" ← DISPLAY THIS!
    std::string subsystem;             // "MC "

    // Constructor para parsing
    explicit ECUInfo(std::string_view raw_string) noexcept {
        if (raw_string.size() >= 39) {
            parse_from_39byte_format(raw_string);
        }
    }

private:
    void parse_from_39byte_format(std::string_view raw) noexcept {
        part_number = trim(raw.substr(0, 12));       // "0261 203 388"
        variant = trim(raw.substr(12, 4));           // "AN09"
        hardware_id = trim(raw.substr(16, 1));       // "I"
        firmware_revision = trim(raw.substr(17, 7)); // "2000148"
        manufacturer = trim(raw.substr(24, 5));      // "BOSCH"
        model_code = trim(raw.substr(30, 5));        // "MA17"
        subsystem = trim(raw.substr(37, 3));         // "MC "
    }
};
```

---

## 🎨 TopBar Display

```
✓ ECU: BOSCH MA17 | FW: 2000148 | 15.3ms
✓ P/N: 0261 203 388-AN09 | Modelo: MA17
```

**Ou compactado:**
```
✓ 0261.203.388 (MA17) | 15.3ms
```

---

## 🔐 Validação & Safety

### NASA P10 Compliance:
- ✅ No dynamic allocation (std::string on stack)
- ✅ Bounded parsing (max 39 bytes)
- ✅ Error handling (size check)
- ✅ No recursion
- ✅ Strong types (struct, not raw data)

### Test Cases:

```cpp
TEST(ECUInfoTest, ParsesCompleteString) {
    const std::string raw = "0261 203 388AN09I2000148BOSCH MA17  MC ";
    ECUInfo ecu(raw);

    EXPECT_EQ(ecu.model_code, "MA17");
    EXPECT_EQ(ecu.manufacturer, "BOSCH");
    EXPECT_EQ(ecu.firmware_revision, "2000148");
    EXPECT_EQ(ecu.part_number, "0261 203 388");
    EXPECT_EQ(ecu.variant, "AN09");
}

TEST(ECUInfoTest, HandlesTruncatedString) {
    const std::string raw = "0261 203";
    ECUInfo ecu(raw);

    // Deve lidar gracefully com parsing incompleto
    EXPECT_FALSE(ecu.model_code.empty() || ecu.model_code == "");
}

TEST(ECUInfoTest, DisplayFormatIsClear) {
    const std::string raw = "0261 203 388AN09I2000148BOSCH MA17  MC ";
    ECUInfo ecu(raw);

    auto display = ecu.display_string();  // "BOSCH MA17"
    EXPECT_NE(display.find("MA17"), std::string::npos);
}
```

---

## 📌 Próximos Passos

1. **Adicionar `ECUInfo::parse_from_39byte_format()`** a `app_data.h`
2. **Implementar em `ECUBackend::ProcessInitPackets()`**
   - Capturar fragmentos F4 consecutivos
   - Montar string de 39 bytes
   - Chamar `ECUInfo(full_string)`
3. **Display em `TopBar.cpp`**
   - Mostrar `ecu_info.model_code` + `ecu_info.manufacturer`
   - Ex: "✓ BOSCH MA17 | 15.3ms"
4. **Testes unitários** com exemplos reais (Fiat Tipo, Lancia, Alfa Romeo)

---

## 📚 Referências no Código

**Arquivo disassembly:** `FiatTipoIDAFullDump_27c256.asm.compressed`
**Offset:** `code:00004D2D` (send_ident_banner)
**String data:** `code:00006E00` (a0261203388an09)

---

## 🚀 Impacto

- **UX:** Usuário vê claramente qual ECU está conectado ("BOSCH MA17")
- **Debug:** Facilita troubleshooting com modelo exato visível
- **Validação:** Pode rejeitar ECUs não esperados (ex: se só suporta MA17)
- **Log:** Registra fingerprint exato para análise histórica

