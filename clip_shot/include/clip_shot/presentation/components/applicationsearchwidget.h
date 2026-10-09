#ifndef CLIP_SHOT_PRESENTATION_COMPONENTS_APPLICATIONSEARCHWIDGET_H
#define CLIP_SHOT_PRESENTATION_COMPONENTS_APPLICATIONSEARCHWIDGET_H

#include "clip_shot/presentation/settings/settingsregistry.h"
#include "clip_shot/presentation/settings/settingssearchindex.h"

#include <QHash>
#include <QString>
#include <QVariant>
#include <QWidget>

class QAbstractItemDelegate;
class QEvent;

namespace clip_shot::presentation::styles {
struct ThemeAliasMetricToken;
struct ThemeColorScheme;
} // namespace clip_shot::presentation::styles
namespace adqt::widgets {
class AdSelect;
}

class ApplicationSearchWidget final : public QWidget {
    Q_OBJECT

  public:
    ApplicationSearchWidget(const clip_shot::presentation::settings::SettingsRegistry& registry,
                            const clip_shot::presentation::styles::ThemeAliasMetricToken& metric,
                            QWidget* parent = nullptr);
    ~ApplicationSearchWidget() override;

    void setPlaceholderText(const QString& text);
    [[nodiscard]] QString query() const;
    void applyTheme(const clip_shot::presentation::styles::ThemeColorScheme& scheme);
    void rebuildIndex();

  signals:
    void locationActivated(const clip_shot::presentation::settings::SettingsLocation& location);

  protected:
    void changeEvent(QEvent* event) override;

  private:
    void populateResults(const QString& query);
    void handleSelectedValue(const QVariant& value, const QString& label);

    clip_shot::presentation::settings::SettingsSearchIndex m_index;
    adqt::widgets::AdSelect* m_select = nullptr;
    QAbstractItemDelegate* m_resultDelegate = nullptr;
    QHash<QString, clip_shot::presentation::settings::SettingsLocation> m_locationsByValue;
    bool m_clearingSelection = false;
};

#endif // CLIP_SHOT_PRESENTATION_COMPONENTS_APPLICATIONSEARCHWIDGET_H
