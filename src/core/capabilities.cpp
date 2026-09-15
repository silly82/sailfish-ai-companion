#include "capabilities.h"

Capabilities::Capabilities(QObject *parent) : QObject(parent) {}

#ifdef SFAI_HARBOUR
bool Capabilities::sandboxed()      const { return true;  }
// QtContacts (libQt5Contacts.so.5) steht nicht in allowed_libraries.conf --
// Harbour-legal ist nur der QML-Import, den es fuer find_contact noch nicht
// gibt (H2, docs/todo-harbour-vs-full.md). Manifest deshalb ehrlich: kein
// Tool, statt es registriert und nur ausgegraut anzuzeigen.
bool Capabilities::contacts()       const { return false; }
// Kein Tool nutzt Telephony (H8, docs/todo-harbour-vs-full.md) -- ein
// Capability-Flag, das nichts freischaltet, ist irrefuehrend.
bool Capabilities::telephony()      const { return false; }
bool Capabilities::messages()       const { return false; }
bool Capabilities::calendar()       const { return false; }
bool Capabilities::filesystem()     const { return true;  }  // nur via Sailfish.Pickers
bool Capabilities::localInference() const { return false; }  // ab M5 evtl. true
// Nemo.KeepAlive haelt eine Streaming-Antwort am Leben, waehrend das Display
// blankt (H8) -- die einzige Form von "Hintergrund", die Harbour erlaubt.
bool Capabilities::background()     const { return true;  }
bool Capabilities::automation()     const { return false; }
#else
bool Capabilities::sandboxed()      const { return false; }
bool Capabilities::contacts()       const { return true;  }
bool Capabilities::telephony()      const { return false; }
bool Capabilities::messages()       const { return true;  }
bool Capabilities::calendar()       const { return true;  }
bool Capabilities::filesystem()     const { return true;  }
bool Capabilities::localInference() const { return true;  }
bool Capabilities::background()     const { return true;  }
bool Capabilities::automation()     const { return true;  }
#endif
