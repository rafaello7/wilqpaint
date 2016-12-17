#ifndef COLORCHOOSER_H
#define COLORCHOOSER_H

enum ChosenColor {
    CC_STROKE,
    CC_FILL
};

void setColorChooseNotifyHandler(void (*)(GtkWidget*, enum ChosenColor));

#endif /* COLORCHOOSER_H */
