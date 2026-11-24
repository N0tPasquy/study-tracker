# Study Tracker TUI ⏱️📚

Applicativo standalone **TUI (Terminal User Interface)** in C++ moderno a zero attrito e keyboard-first in stile **Neovim**, progettato per seguire e completare la roadmap di preparazione per l'esame di **"Matematica II + Probabilità"**.

L'applicazione si avvia all'istante sull'ultimo blocco di studio in corso, permette di tracciare il timer per segmento, spuntare i task della checklist, prendere appunti veloci e salvare automaticamente ogni progresso in modo atomico sul file `roadmap_matematica2.json`.

---

## ✨ Funzionalità Principali

* **Workflow Modale in stile Neovim:** 
  - Modalità `NORMAL` per la navigazione rapida e il controllo con i tasti della *home row*.
  - Modalità `INSERT` per digitare liberamente note e dubbi sul campo `last_note`.
* **Timer per Segmenti (Pomodoro avanzato):**
  - Tracciamento differenziato per segmenti di **Studio** vs **Pausa** (Teoria, Esercizi, Revisione, ecc.).
  - Cronometro in tempo reale con avviso acustico e visivo al completamento del target.
  - Supporto per tempo supplementare (*Overtime*).
  - Messa in pausa automatica quando si cambia blocco o segmento.
* **Top Bar Informativa:**
  - Countdown in giorni all'esame.
  - Obiettivo settimanale dinamico (Lunedì–Domenica, standard ISO 8601).
  - Barra di progresso globale completamento roadmap.
* **Navigatore Roadmap Completo (42 Blocchi):**
  - Vista toggleabile con `r` o `Tab`.
  - Scorrimento fluido con viewport centrata, salto rapido con `gg`/`G` e mezza pagina con `Ctrl+d`/`Ctrl+u`.
* **Persistenza Atomica e Sicura:**
  - Salvataggio automatico continuo (su toggle task, cambio segmento/blocco, stop timer, periodico ogni 30s ed uscita).
  - Scrittura atomica tramite file temporaneo `.tmp` e rinomina per garantire integrità anche in caso di chiusura improvvisa del terminale.

---

## ⌨️ Tabella Scorciatoie (Cheat Sheet)

### Modalità `NORMAL`

| Tasto | Azione |
| :--- | :--- |
| **`r` / `Tab`** | Alterna Vista Focus <-> Vista Roadmap |
| **`w` / `Ctrl+S`** | Forza salvataggio immediato |
| **`q`** | Chiude Roadmap View (se aperta) oppure salva ed esce dall'app |
| **`ZZ` / `Ctrl+Q`** | Salva ed esce immediatamente da qualsiasi vista |
| **`j` / `k`** (o `↓` / `↑`) | Muove la selezione (nella checklist task o nella lista roadmap) |
| **`x` / `Space` / `Enter`** | Spunta / despunta il task selezionato (in Focus View) |
| **`h` / `l`** (o `[` / `]`) | Segmento precedente / successivo nel blocco corrente |
| **`H` / `L`** | Blocco precedente / successivo nella roadmap |
| **`t`** | Avvia / Mette in pausa il timer del segmento attivo |
| **`c`** | Segna il segmento attivo come completato e avanza al successivo |
| **`R`** | Azzera il cronometro del segmento attivo |
| **`M`** | Marca / riapre l'intero blocco come completato (aggiorna la data) |
| **`i`** | Entra in **`INSERT` mode** per modificare `last_note` |
| **`a`** | Entra in **`INSERT` mode** posizionando il cursore a fine nota |
| **`gg` / `G`** | *(In Roadmap)* Salta al primo / ultimo blocco |
| **`Ctrl+d` / `Ctrl+u`** | *(In Roadmap)* Scorre giù / su di mezza pagina |
| **`Enter` / `l`** | *(In Roadmap)* Seleziona il blocco ed entra in Focus View |
| **`Esc`** | *(In Roadmap)* Annulla e torna al blocco corrente |

### Modalità `INSERT` (`last_note`)

| Tasto | Azione |
| :--- | :--- |
| **Qualsiasi tasto** | Inserisce normalmente il testo nella nota |
| **`Esc` / `Ctrl+[`** | Salva la nota e torna in modalità **`NORMAL`** |
| **`Enter`** | Conferma la nota, salva su disco e torna in modalità **`NORMAL`** |
| **`Ctrl+S`** | Salva le modifiche su disco rimanendo in **`INSERT`** |

---

## 🛠️ Come Compilare ed Eseguire

### Prerequisiti
* **Compilatore C++:** C++20 supportato (Clang $\ge$ 13 o GCC $\ge$ 11 o Apple Clang).
* **CMake:** $\ge$ 3.16.
* Tutte le librerie esterne ([FTXUI](https://github.com/ArthurSonzogni/FTXUI) e [nlohmann/json](https://github.com/nlohmann/json)) vengono scaricate e compilate automaticamente da CMake tramite `FetchContent`.

### Compilazione

```bash
# Configura la build
cmake -B build

# Compila l'eseguibile e i test
cmake --build build
```

### Esecuzione dei Test
```bash
./build/tracker_tests
```

### Avvio dell'Applicazione
```bash
# Avvio standard (usa roadmap_matematica2.json)
./build/study_tracker

# Oppure specificando un percorso JSON alternativo:
./build/study_tracker /percorso/a/roadmap.json
```

---

## 🚀 Creare il comando rapido `mat2` nel terminale

Per avviare l'applicazione in qualsiasi momento digitando semplicemente `mat2` nel terminale (da qualsiasi cartella ti trovi):

1. Apri la configurazione della tua shell (su macOS con zsh è `~/.zshrc`):
   ```bash
   nano ~/.zshrc
   # oppure con Neovim: nvim ~/.zshrc
   ```

2. Aggiungi questa riga in fondo al file:
   ```bash
   alias mat2="/Users/afp316/Documents/study-tracker/build/study_tracker /Users/afp316/Documents/study-tracker/roadmap_matematica2.json"
   ```

3. Ricarica la configurazione del terminale:
   ```bash
   source ~/.zshrc
   ```

Ora puoi digitare semplicemente:
```bash
mat2
```
e lo Study Tracker si aprirà a schermo intero all'istante!
