#include "clipboard.h"

#include "project/clip.h"
#include "project/effect.h"

int clipboard_type = CLIPBOARD_TYPE_CLIP;
QVector<void*> clipboard;
QVector<Transition*> clipboard_transitions;

void clear_clipboard() {
	for (int i=0;i<clipboard.size();i++) {
		if (clipboard_type == CLIPBOARD_TYPE_CLIP) {
			delete static_cast<Clip*>(clipboard.at(i));
		} else if (clipboard_type == CLIPBOARD_TYPE_EFFECT) {
			delete static_cast<Effect*>(clipboard.at(i));
		}
	}
	for (int i=0;clipboard_transitions.size();i++) {
		delete clipboard_transitions.at(i);
	}
	clipboard.clear();
}
