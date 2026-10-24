// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
pragma Singleton

import QtQml

// App → system requests ("intents"). Apps never reach into the shell; they
// ask for an outcome and the shell decides how to fulfil it. Actions:
//
//   app.open        { id, page? }     open or focus an app (optionally a page)
//   session.lock    {}                lock the session
//   shell.overview  {}                show Overview
//   shell.beam      { query? }        open Beam
//   shell.space     { index }         switch Space
QtObject {
    signal requested(string action, var args)

    function request(action: string, args: var) {
        requested(action, args === undefined ? ({}) : args);
    }

    function openApp(id: string, args: var) {
        const payload = Object.assign({ "id": id }, args === undefined ? {} : args);
        request("app.open", payload);
    }
}
