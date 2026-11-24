# Specifiche Tecniche — Study Tracker TUI (C++ / FTXUI)

## 1. Obiettivo del Progetto

Realizzare un applicativo standalone **TUI (Terminal User Interface)** in C++ leggero, keyboard-first in stile **Neovim** e a zero attrito per seguire la roadmap di preparazione all'esame **"Matematica II + Probabilità"**.

Il file `roadmap_matematica2.json` è già pronto ed è la **single source of truth**:

* L'app **non** deve implementare funzionalità CRUD per creare/modificare la struttura della roadmap (blocchi, titoli, segmenti o lista task).

* L'app deve aprirsi istantaneamente sull'ultimo blocco in corso (`current_block_index`) in modalità `NORMAL`, per permettere di riprendere lo studio in meno di 5 secondi, tracciare il timer per segmento, spuntare i task, annotare dove si è rimasti e salvare automaticamente i progressi sul JSON.

## 2. Stack Tecnico e Dipendenze

* **Linguaggio:** C++20 (o C++17 con `<filesystem>` e `<chrono>`).

* **Build System:** CMake ($\ge$ 3.16) con `FetchContent` per scaricare automaticamente le dipendenze (nessuna libreria grafica o dipendenza di sistema richiesta):

  1. **FTXUI** (`ArthurSonzogni/FTXUI`, v5.x o ultima release stabile) — per l'interfaccia terminale.

  2. **nlohmann/json** (`nlohmann/json`, v3.11.x) — per deserializzazione e serializzazione JSON.

* **Target:** singolo eseguibile standalone (es. `study_tracker`).

* **Percorso file JSON:** accettare un percorso opzionale da riga di comando (`./study_tracker [path/to/roadmap.json]`), usando `./roadmap_matematica2.json` come default.

## 3. Schema Dati (`roadmap_matematica2.json`) e Mapping C++

Il file contiene attualmente **42 blocchi** divisi in **7 fasi** (`phase` da `0` a `6`).

### Dettagli critici dello schema da rispettare

1. **Numero variabile di segmenti:** la maggior parte dei blocchi ha 5 segmenti (`Teoria` 60m, `Pausa` 15m, `Esercizi` 120m, `Pausa` 15m, `Revisione e Orale` 60m), ma alcuni blocchi (es. le simulazioni #38 e #40) hanno **3 segmenti**. Non fare mai assunzioni hardcoded sul numero di `segments` o di `tasks`.

2. **Encoding UTF-8:** i testi contengono caratteri accentati italiani e simboli Unicode (`Δ`, `ρ`, `ª`, `°`). Gestire tutte le stringhe come UTF-8 nativo.

3. **Campo `completed_at`:** stringa vuota `""` se il blocco non è completato, oppure data ISO `"YYYY-MM-DD"` al momento del completamento.

### Struct C++ di riferimento

Usare `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT` per evitare errori su eventuali campi omessi:

```cpp
struct Segment {
    std::string label;
    std::string type = "study"; // "study" | "break"
    int target_minutes = 60;
    int elapsed_seconds = 0;
    bool done = false;
};

struct Task {
    std::string text;
    bool done = false;
};

struct Block {
    int id = 0;
    int phase = 0;
    std::string module;
    std::string title;
    bool completed = false;
    std::string completed_at; // "" oppure "YYYY-MM-DD"
    int current_segment_index = 0;
    std::vector<Segment> segments;
    std::vector<Task> tasks;
    std::string last_note;
};

struct Roadmap {
    std::string exam_name;
    std::string exam_date; // "YYYY-MM-DD"
    int weekly_goal_blocks = 3;
    int current_block_index = 0;
    std::vector<Block> blocks;
};
```

### Persistenza sicura (Atomic Save)

Per evitare file troncati o corrotti in caso di chiusura improvvisa del terminale:

1. Serializzare il JSON indentato (`dump(2)`) su un file temporaneo nella stessa directory (`<path>.tmp`).

2. Fare flush e chiudere lo stream.

3. Sostituire il file originale con `std::filesystem::rename`.

**Trigger di salvataggio automatico:**

* Al toggle di qualsiasi `task.done` o `block.completed`.

* Al cambio di blocco (`current_block_index`) o di segmento (`current_segment_index`).

* Alla pausa/completamento/reset di un segmento del timer.

* Periodicamente (es. ogni 30 secondi) mentre il timer è attivo.

* All'uscita dalla modalità `INSERT` (dopo aver modificato `last_note`) e sempre alla chiusura dell'app.

## 4. Specifiche UX/UI (FTXUI)

L'interfaccia gira a schermo intero (`ScreenInteractive::Fullscreen()`) con controllo **keyboard-first in stile Neovim** (mantenendo comunque compatibilità con frecce e mouse).

### A. Intestazione Fissa (Top Bar)

Sempre visibile in alto:

* **Titolo esame** (`exam_name`).

* **Countdown all'esame:** giorni mancanti rispetto a `exam_date` (calcolati dalla data locale di sistema).

* **Obiettivo settimanale:** contatore `X / weekly_goal_blocks` dei blocchi con `completed == true` la cui data `completed_at` cade nella settimana corrente (Lunedì–Domenica).

* **Barra di progresso globale (`gauge`):** percentuale e conteggio `completati / totale` (es. `4/42 — 9%`).

### B. Vista Principale — Focus sul Blocco Corrente (schermata di avvio)

Mostra subito `blocks[current_block_index]`:

1. **Intestazione blocco:**

   * Badge colorato in base a `phase` (0..6) con nome del `module`.

   * ID e `title` del blocco (es. `[#1] Ripasso veloce (limiti e matrici)`).

   * Stato completamento (`IN CORSO` / `COMPLETATO`), attivabile da tastiera (`M`) o bottone (aggiornando `completed_at` alla data odierna ISO).

2. **Pannello Segmenti e Timer:**

   * Elenco visivo dei `segments` del blocco che evidenzia il segmento attivo (`current_segment_index`), lo stato (`[x]` / `[>]` / `[ ]`), la `label`, il tempo `MM:SS / MM:SS` e una differenziazione cromatica tra `"study"` e `"break"`.

   * **Cronometro grande/ben leggibile** del segmento attivo (tempo trascorso / target e barra di avanzamento del segmento).

   * **Controlli timer** (tutti raggiungibili da tastiera senza spostare le mani dalla home row, oltre che cliccabili):

     * `Avvia / Pausa` (`t`)

     * `Segmento Precedente` / `Segmento Successivo` (`h` / `l`)

     * `Completa Segmento` (`c` — marca `done = true` e avanza al successivo se presente)

     * `Reset Segmento` (`R` — azzera `elapsed_seconds` del segmento corrente)

   * Quando `elapsed_seconds >= target_minutes * 60`: marcare automaticamente `done = true`, mettere in pausa il timer, emettere un beep terminale (`\a`) e mostrare un avviso visivo (es. *"Segmento completato! Premi l per passare al successivo"*), senza far partire automaticamente il segmento seguente a sorpresa.

   * Riepilogo compatto del tempo totale svolto sul blocco (distinguendo studio effettivo vs pause).

3. **Checklist (`tasks`):**

   * Lista navigabile con `j` / `k` e spuntabile con `x`, `Space` o `Enter` (oltre che col mouse).

   * Conteggio rapido (es. `2/4 completati`).

4. **Casella "Ultimo punto / Dubbi" (`last_note`):**

   * Campo di input FTXUI dedicato dove scrivere rapidamente a che esercizio ci si è fermati o quali dubbi rivedere.

   * In `NORMAL` mode è in sola lettura (con bordo discreto); premendo `i` o `a` (o cliccandoci) si attiva la modalità `INSERT` (bordo evidenziato) per scrivere liberamente, e `Esc` salva e riporta in `NORMAL` mode.

### C. Vista Secondaria — Navigatore Roadmap (Toggleabile)

Accessibile con `r` o `Tab`:

* Lista navigabile di tutti i 42 blocchi con `j`/`k`, `gg`/`G` e `Ctrl+d`/`Ctrl+u`.

* Mostra fase/modulo, stato (`[x]` completato, `[>]` corrente, `[ ]` da fare) e avanzamento task.

* Premendo `Enter` (o `l`) su un blocco, aggiorna `current_block_index`, salva e torna subito alla Vista Principale su quel blocco.

* Premendo `Esc` o `q` (o `r`/`Tab`) chiude la vista roadmap e torna alla Vista Principale senza cambiare blocco (comportamento stile floating window di Neovim).

### D. Footer (Statusline stile Neovim)

Una barra compatta in basso ispirata alla statusline di Neovim (`lualine`):

* **Sinistra:** badge colorato della modalità corrente (` NORMAL ` in azzurro/verde, ` INSERT ` in giallo/arancione) + nome vista attiva (`FOCUS` / `ROADMAP`).

* **Centro:** legenda sintetica delle scorciatoie contestuali alla modalità/vista attiva.

* **Destra:** stato timer (`TIMER ON` / `PAUSA`) e stato salvataggio (`Salvato` / `Modificato`).

## 5. Sistema Modale e Scorciatoie da Tastiera (Stile Neovim)

L'app gestisce esplicitamente uno stato `enum class AppMode { Normal, Insert }`:

* In **`Normal` mode**, un `CatchEvent` di livello radice intercetta le scorciatoie Neovim prima che arrivino ai widget, garantendo risposta immediata e prevedibile ovunque si trovi il focus.
* In **`Insert` mode**, tutti i caratteri stampabili, `Space`, `Backspace` e frecce vengono inoltrati esclusivamente al componente `Input` di `last_note`; nessuna scorciatoia a tasto singolo è attiva. `Esc` (o `Ctrl+[`) salva la nota e riporta immediatamente a `Normal` mode.

### Mappa Tasti — Modalità `NORMAL`

| Contesto | Tasto | Azione |
| :--- | :--- | :--- |
| **Globale** | `r` / `Tab` | Alterna Vista Focus <-> Vista Roadmap |
| **Globale** | `w` / `Ctrl+S` | Forza salvataggio immediato su JSON (stile `:w`) |
| **Globale** | `q` | Se in Vista Roadmap torna a Focus; se in Focus salva ed esce |
| **Globale** | `ZZ` / `Ctrl+Q` | Salva ed esci immediatamente da qualsiasi vista |
| **Focus View** | `j` / `k` (o `↓` / `↑`) | Muove la selezione nella checklist dei task |
| **Focus View** | `x` / `Space` / `Enter` | Spunta / despunta il task selezionato |
| **Focus View** | `h` / `l` (o `[` / `]`) | Segmento precedente / successivo nel blocco corrente |
| **Focus View** | `H` / `L` | Blocco precedente / successivo nella roadmap |
| **Focus View** | `t` | Avvia / Pausa il timer del segmento attivo |
| **Focus View** | `c` | Marca il segmento attivo come completato (`done = true`) e avanza al successivo |
| **Focus View** | `R` | Reset del timer (`elapsed_seconds = 0`) del segmento attivo |
| **Focus View** | `M` | Marca / riapre l'intero blocco come completato (imposta/svuota `completed_at`) |
| **Focus View** | `i` / `a` | Entra in **`INSERT` mode** sul campo `last_note` (`a` posiziona il cursore in fondo) |
| **Roadmap View** | `j` / `k` (o `↓` / `↑`) | Scorre i blocchi giù / su |
| **Roadmap View** | `gg` / `G` | Salta al primo (`#1`) / ultimo (`#42`) blocco |
| **Roadmap View** | `Ctrl+d` / `Ctrl+u` | Scorre mezza pagina giù / su |
| **Roadmap View** | `Enter` / `l` | Seleziona il blocco, salva e torna alla Focus View |
| **Roadmap View** | `Esc` / `q` | Chiude la Roadmap View e torna alla Focus View corrente |

### Mappa Tasti — Modalità `INSERT` (editing `last_note`)

| Tasto | Azione |
| :--- | :--- |
| Qualsiasi carattere / `Space` | Inserisce normalmente il testo in `last_note` |
| `Esc` / `Ctrl+[` | Esce in **`NORMAL` mode** e salva automaticamente il JSON |
| `Ctrl+S` | Forza salvataggio restando in `INSERT` mode |

> **Nota implementativa per sequenze a 2 tasti (`gg`, `ZZ`):** gestire un semplice `pending_key` (`char` + timestamp o reset al tasto successivo) dentro il gestore `CatchEvent` in `Normal` mode: se `pending_key == 'g'` e arriva `'g'`, esegue salto in cima e azzera `pending_key`; analogo per `'Z'` + `'Z'`.

## 6. Gestione Concorrenza del Timer (FTXUI)

Per aggiornare il timer ogni secondo senza bloccare la UI né consumare CPU:

* Usare un `std::thread` dedicato controllato da `std::atomic<bool> app_running{true}` e `std::atomic<bool> timer_running{false}`.

* Aggiornare lo stato condiviso tramite `screen.Post(...)` sul thread UI di FTXUI (approccio consigliato e privo di data race).

* Nel thread timer: dormire a intervalli brevi (es. 100 ms) controllando `app_running` per una chiusura istantanea, e al passare di 1 secondo reale (usando `std::chrono::steady_clock`) inviare `screen.Post([&]{ ... })` + `screen.PostEvent(Event::Custom)` per forzare il redraw.

* Al termine di `screen.Loop(...)`, impostare `app_running = false`, fare `join()` sul thread ed eseguire il salvataggio finale.

## 7. Struttura Consigliata della Repository

Mantenere l'architettura snella e manutenibile (niente over-engineering):

```text
study-tracker/
├── CMakeLists.txt
├── roadmap_matematica2.json
└── src/
    ├── main.cpp        # Entry point, CLI args, signal/exit handling, avvio UI + timer thread
    ├── model.hpp       # Struct Roadmap/Block/Segment/Task + macro nlohmann/json + helper date/settimana
    ├── storage.hpp     # Caricamento e salvataggio atomico JSON
    ├── storage.cpp
    ├── ui.hpp          # Costruzione componenti FTXUI (Focus View + Roadmap View + AppMode Normal/Insert)
    └── ui.cpp
```

## 8. Checklist di Verifica Finale (Acceptance Criteria)

Prima di considerare completo il progetto, verificare che:

1. Il progetto compili senza warning rilevanti (`-Wall -Wextra`) con CMake + `FetchContent`.

2. Il caricamento di `roadmap_matematica2.json` funzioni senza errori e preservi tutti i 42 blocchi, inclusi quelli con 3 segmenti (es. blocco 38 e 40) e tutti i caratteri UTF-8.

3. Spuntare un task, scrivere in `last_note`, far scorrere il timer e chiudere/riaprire l'app ripristini esattamente stato, blocco corrente, segmento attivo e secondi trascorsi.

4. La navigazione Neovim funzioni correttamente: in `NORMAL` mode `j`/`k`/`h`/`l`/`H`/`L`/`t`/`c`/`r`/`gg`/`G`/`Ctrl+d`/`Ctrl+u` rispondano subito; premendo `i`/`a` si passi a `INSERT` mode potendo digitare liberamente `j`, `k`, `q`, `t`, `r`, `Space` dentro `last_note`; premendo `Esc` si torni in `NORMAL` mode salvando la nota.

5. Il contatore dell'obiettivo settimanale (`weekly_goal_blocks`) si aggiorni correttamente quando un blocco viene marcato completato con `M`.

6. La chiusura con `q` (in Focus View), `ZZ` o `Ctrl+Q` termini il thread del timer in modo pulito e scriva il JSON in modo atomico.