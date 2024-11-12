#ifndef DPIHELPER_H
#define DPIHELPER_H

#include <QApplication>
#include <QScreen>
#include <QWindow>
#include <QDebug>
#include <QWidget>

class DPIHelper {
public:
    static void SetApplicationDPI() {
        // 获取主屏幕
        QScreen *screen { QGuiApplication::primaryScreen()};
        if (!screen) return;

        // 获取屏幕逻辑DPI
        qreal dpi { screen->logicalDotsPerInch()};

        // 获取屏幕物理DPI
        qreal physicalDpi { screen->physicalDotsPerInch()};

        // 计算缩放因子
        qreal scaleFactor { physicalDpi / 96}; // 96 DPI是Windows的标准DPI

        // 设置应用程序级别的缩放因子
        qputenv("QT_SCALE_FACTOR", QString::number(scaleFactor).toLatin1());

        QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
            Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    }

    static void AdjustWidgetDPI(QWidget* widget) {
        if (!widget) return;

        // 获取窗口所在屏幕
        QScreen* screen { widget->window()->screen()};
        if (!screen) return;

        // 获取当前屏幕DPI
        qreal dpi { screen->logicalDotsPerInch()};

        // 调整字体大小
        QFont font { widget->font()};
        int font_size { static_cast<int>(font.pointSize() * (dpi / 96.0))};
        font.setPointSize(font_size);
        widget->setFont(font);

        // 递归处理所有子控件
        for (QObject* child : widget->children()) {
            if (QWidget* child_widget = qobject_cast<QWidget*>(child)) {
                AdjustWidgetDPI(child_widget);
            }
        }
    }
};

#endif // DPIHELPER_H
