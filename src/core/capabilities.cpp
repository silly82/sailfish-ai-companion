#include "capabilities.h"

Capabilities::Capabilities(QObject *parent) : QObject(parent) {}

#ifdef SFAI_HARBOUR
bool Capabilities::sandboxed()      const { return true;  }
bool Capabilities::contacts()       const { return true;  }  // Perm: Contacts
bool Capabilities::telephony()      const { return true;  }  // Sailfish.Telephony (Umfang prüfen)
bool Capabilities::messages()       const { return false; }
bool Capabilities::calendar()       const { return false; }
bool Capabilities::filesystem()     const { return true;  }  // nur via Sailfish.Pickers
bool Capabilities::localInference() const { return false; }  // ab M5 evtl. true
bool Capabilities::background()     const { return false; }  // nur Nemo.KeepAlive
bool Capabilities::automation()     const { return false; }
#else
bool Capabilities::sandboxed()      const { return false; }
bool Capabilities::contacts()       const { return true;  }
bool Capabilities::telephony()      const { return true;  }
bool Capabilities::messages()       const { return true;  }
bool Capabilities::calendar()       const { return true;  }
bool Capabilities::filesystem()     const { return true;  }
bool Capabilities::localInference() const { return true;  }
bool Capabilities::background()     const { return true;  }
bool Capabilities::automation()     const { return true;  }
#endif
