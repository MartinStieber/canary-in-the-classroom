# common

Sdílený kód a definice protokolu pro obě firmwary.

- `Specifikace zpráv ESP-NOW panel ↔ uzel.pdf`: popis zpráv REQ/RESP/HIST, význam polí, pravidla pro uzel a příklady zpráv po bajtech.
- `let_proto.h`: hlavičkový soubor ze specifikace. Includují ho oba firmwary (`#include "let_proto.h"`), takže formát nemůže mezi stranami nesedět. `static_assert` zastaví překlad, kdyby se změnila velikost struktur.

Jediný zdroj pravdy pro formát zpráv. Obě strany musí používat stejnou verzi.
Při změně formátu zvyš `LET_VERSION`, uprav PDF i `let_proto.h` a dej vědět druhé straně (pull request).
