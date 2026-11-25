# Study Tracker TUI

Un'applicazione terminale (**TUI**) standalone in C++ per tracciare lo studio e la preparazione dell'esame di **Matematica II + Probabilità**.

L'interfaccia e l'esperienza d'uso prendono forte ispirazione da **Neovim**: workflow modale (`NORMAL` e `INSERT` mode), navigazione rapida da tastiera (*keyboard-first*) con i tasti `h`/`j`/`k`/`l` e zero distrazioni.

## Funzionalità
- **Controlli stile Neovim:** navigazione e comandi in modalità `NORMAL`, scrittura note in modalità `INSERT`.
- **Timer per segmenti:** suddivisione per blocchi di studio e pause, con conteggio overtime.
- **Checklist e Statistiche:** spunta task, avanzamento globale, obiettivo settimanale e countdown all'esame.
- **Sezione Risorse / Video:** videolezioni e playlist YouTube collegate a ciascun blocco, con stato visto (`[✔]`), badge (`[VID]`, `[PL]`) e apertura sicura nel browser di sistema senza corrompere il terminale.
- **Salvataggio atomico:** sincronizzazione automatica dei progressi su `roadmap_matematica2.json`.

---

## Controlli da Tastiera

| Tasto | Vista / Contesto | Azione |
| :--- | :--- | :--- |
| `Tab` / `Shift+Tab` | FOCUS | Alterna il focus tra **Tasks** e **Risorse** |
| `j` / `k` (o `↓` / `↑`) | Tasks / Risorse | Scorri gli elementi della sezione attiva |
| `x` / `Space` | Tasks | Spunta il task attivo |
| `x` / `Space` | Risorse | Segna come visto/non visto (`watched`) |
| `Enter` o `o` | Risorse | Apre il link nel browser di sistema |
| `h` / `l` (o `[` / `]`) | FOCUS | Segmento timer precedente / successivo |
| `H` / `L` | FOCUS | Blocco precedente / successivo |
| `t` | FOCUS | Avvia / Pausa timer |
| `c` / `R` | FOCUS | Completa/Riapri segmento / Reset timer segmento |
| `M` | FOCUS | Marca l'intero blocco come completato |
| `i` / `a` | FOCUS | Entra in modalità `INSERT` per modificare la nota |
| `r` | Globale | Apri / Chiudi vista Roadmap completa |
| `w` / `Ctrl+S` | Globale | Salvataggio immediato su disco |
| `ZZ` / `Ctrl+Q` / `q` | Globale | Salva ed esci |

---

## Struttura della Roadmap

Il file `roadmap_matematica2.json` incluso nel repository è costruito a partire dal sillabo universitario di Analisi Matematica II e Calcolo delle Probabilità, organizzato in **42 blocchi da 4 ore nette di studio** divisi in 7 fasi progressive:

* **Fase 0 (Prerequisiti):** recupero propedeutico di limiti notevoli, sviluppi di Taylor, integrali in una variabile e basi di algebra lineare.
* **Fasi 1–5 (Programma d'esame):** serie numeriche e di potenze, calcolo differenziale e ottimizzazione in più variabili, EDO, integrali curvilinei e doppi, calcolo delle probabilità.
* **Fase 6 (Simulazioni e Orale):** prove scritte cronometrate con revisione degli errori e ripasso di teoremi e contro-esempi.

Ogni blocco standard è suddiviso in 5 segmenti (`Teoria 60m` → `Pausa 15m` → `Esercizi 120m` → `Pausa 15m` → `Revisione 60m`). La struttura è completamente personalizzabile: modificando il JSON puoi adattare l'app a qualsiasi altro esame.

---

## Come Compilare ed Eseguire

### Prerequisiti
- Compilatore C++ con supporto C++20 (Apple Clang, Clang $\ge$ 13 o GCC $\ge$ 11)
- CMake $\ge$ 3.16

*(Le dipendenze FTXUI e nlohmann/json vengono scaricate e compilate in automatico tramite CMake)*

### Compilazione
```bash
cmake -B build
cmake --build build
```

### Esecuzione
```bash
./build/study_tracker
./build/study_tracker --help
```

Per eseguire i test unitari e di integrazione (con doctest / CTest):
```bash
ctest --test-dir build --output-on-failure
# oppure direttamente:
./build/tracker_tests
```
