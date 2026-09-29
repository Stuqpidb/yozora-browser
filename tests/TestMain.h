// SPDX-License-Identifier: MIT
//
// Shared entry point for the Yozora tests.
//
// QTEST_MAIN creates a QApplication without the attributes Qt WebEngine
// requires, and the test binaries link the same static library as the browser.
// Setting the attributes here keeps the tests from tripping over WebEngine's
// static initialisation.

#pragma once

#include <QApplication>
#include <QTest>

namespace yozora::testing {

template <typename TestObject>
int run(TestObject& test, int argc, char** argv)
{
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts, true);
    QApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("Yozora"));
    app.setApplicationName(QStringLiteral("YozoraTests"));

    return QTest::qExec(&test, argc, argv);
}

}  // namespace yozora::testing
