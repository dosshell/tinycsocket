from vboxapi import VirtualBoxManager
import time


def waitForGuest(mgr, session, username, password, timeout=120):
    guest = None
    for n in range(timeout):
        try:
            guest = session.console.guest.createSession(username, password, '', '')
            s = guest.waitFor(mgr.constants.GuestSessionWaitForFlag_Start, 1000)
            if s == 2:  # bug fix
                s = guest.waitFor(mgr.constants.GuestSessionWaitForFlag_Start, 1000)
            if s == 1:
                break

            guest.close()
            guest = None
            time.sleep(1)
        except Exception:
            pass
    return guest


def run(image, snapshot, user, password):
    mgr = VirtualBoxManager(None, None)
    vbox = mgr.getVirtualBox()
    machine = vbox.findMachine(image)
    snap = machine.findSnapshot(snapshot)
    session = mgr.getSessionObject(vbox)
    machine.lockMachine(session, mgr.constants.LockType_Write)
    snap_progress = session.machine.restoreSnapshot(snap)
    snap_progress.waitForCompletion(-1)
    session.unlockMachine()
    progress = machine.launchVMProcess(session, "gui", [])
    progress.waitForCompletion(-1)
    guest = waitForGuest(mgr, session, user, password)
    guest.close()
    session.console.powerButton()
    mgr.closeMachineSession(session)


def main():
    run('xp', 'snappy', 'Administrator', 'abc123')


if __name__ == "__main__":
    main()
