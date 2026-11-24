# Study Tracker TUI

Un'applicazione terminale (**TUI**) standalone in C++ per tracciare lo studio e la preparazione dell'esame di **Matematica II + Probabilità**.

L'interfaccia e l'esperienza d'uso prendono forte ispirazione da **Neovim**: workflow modale (`NORMAL` e `INSERT` mode), navigazione rapida da tastiera (*keyboard-first*) con i tasti `h`/`j`/`k`/`l` e zero distrazioni.

## Funzionalità
- **Controlli stile Neovim:** navigazione e comandi in modalità `NORMAL`, scrittura note in modalità `INSERT`.
- **Timer per segmenti:** suddivisione per blocchi di studio e pause, con conteggio overtime.
- **Checklist e Statistiche:** spunta task, avanzamento globale, obiettivo settimanale e countdown all'esame.
- **Salvataggio atomico:** sincronizzazione automatica dei progressi su `roadmap_matematica2.json`.

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
```

Per eseguire i test unitari e di integrazione:
```bash
./build/tracker_tests
```
