/*
 * 本文件是NetAssist的一部分。
 *
 * NetAssist是一个自由软件：你可以根据自由软件基金会发布的GNU通用公共许可证的条款，重新分发和/或修改它，既可以是许可证的第3版，也可以（根据你的选择）是任何更高版本。
 *
 * NetAssist的发布目的是希望它有用，但没有任何担保；甚至没有适销性或特定用途适用性的隐含担保。有关详细信息，请参阅GNU通用公共许可证。
 *
 * 你应该已经收到一份GNU通用公共许可证的副本。如果没有，请参阅<https://www.gnu.org/licenses/>。
 */
#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
