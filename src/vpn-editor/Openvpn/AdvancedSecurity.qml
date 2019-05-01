/*
 * Copyright (C) 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.4
import Ubuntu.Components 1.3
import Ubuntu.Components.ListItems 1.3 as ListItems

Page {
    property var connection

    title: i18n.tr("Security")

    Flickable {
        anchors.fill: parent
        contentHeight: contentItem.childrenRect.height
        boundsBehavior: (contentHeight > root.height) ?
                            Flickable.DragAndOvershootBounds :
                            Flickable.StopAtBounds
        flickableDirection: Flickable.VerticalFlick

        Column {
            anchors.left: parent.left
            anchors.right: parent.right

            ListItems.ItemSelector {
                text: i18n.tr("Cipher:")
                model: [
                    i18n.tr("Default"),
                    i18n.tr("DES-CBC"),
                    i18n.tr("RC2-CBC"),
                    i18n.tr("DES-EDE-CBC"),
                    i18n.tr("DES-EDE3-CBC"),
                    i18n.tr("DESX-CBC"),
                    i18n.tr("RC2-40-CBC"),
                    i18n.tr("CAST5-CBC"),
                    i18n.tr("AES-128-CBC"),
                    i18n.tr("AES-192-CBC"),
                    i18n.tr("AES-256-CBC"),
                    i18n.tr("CAMELLIA-128-CBC"),
                    i18n.tr("CAMELLIA-192-CBC"),
                    i18n.tr("CAMELLIA-256-CBC"),
                    i18n.tr("SEED-CBC"),
                    i18n.tr("AES-128-CBC-HMAC-SHA1"),
                    i18n.tr("AES-256-CBC-HMAC-SHA1"),
                    i18n.tr("NONE"),
                ]
                selectedIndex: connection.cipher
                onSelectedIndexChanged: connection.cipher = selectedIndex
            }

            OptionalValue {
                text: i18n.tr("Use cipher key size:")

                id: keysizeSetCheckbox
                Binding {target: keysizeSetCheckbox; property: "checked"; value: connection.keysizeSet}
                onCheckedChanged: connection.keysizeSet = checked

                control: TextField {
                    text: connection.keysize
                    onTextChanged: connection.keysize = parseInt(text) || 0
                    enabled: connection.keysizeSet
                    validator: IntValidator{bottom: 0}
                    width: units.gu(10)
                    inputMethodHints: Qt.ImhDigitsOnly
                }
            }

            ListItems.ItemSelector {
                text: i18n.tr("HMAC authentication:")
                model: [
                    i18n.tr("Default"),
                    i18n.tr("None"),
                    i18n.tr("RSA MD-4"),
                    i18n.tr("MD-5"),
                    i18n.tr("SHA-1"),
                    i18n.tr("SHA-224"),
                    i18n.tr("SHA-256"),
                    i18n.tr("SHA-384"),
                    i18n.tr("SHA-512"),
                    i18n.tr("RIPEMD-160")
                ]
                selectedIndex: connection.auth
                onSelectedIndexChanged: connection.auth = selectedIndex
            }
        }
    }
}
