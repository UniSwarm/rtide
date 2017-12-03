#ifndef SETTINGSPAGE_H
#define SETTINGSPAGE_H

#include <QWidget>
#include <QIcon>

class SettingsPage : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsPage(const QIcon &icon=QIcon(), const QString &label=QString());

    const QIcon &icon() const;
    void setIcon(const QIcon &icon);

    const QString &label() const;
    void setLabel(const QString &label);

public slots:
    void commit();

protected:
    QIcon _icon;
    QString _label;

    virtual void execCommit() =0;
    virtual void createWidgets() =0;
};

#endif // SETTINGSPAGE_H
