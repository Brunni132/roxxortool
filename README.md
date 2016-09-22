# The Roxxor Tool

~Many small tools to improve your workflow on Windows.~

I decided to publicly release a tool that I've been developing and improving ever since I have got a computer. I have a little notstalgic love for Windows, I know it isn't common, but anyway. And let's admit it, this OS is far from perfect in its default configuration. But it is fairly easy to implement the things missing by yourself.

This tool focuses on very basic functionality that is usually not covered by other tools (else I would simply have used them in the first place). So let's go on with a list of tweaks, associated with their key (which you'll use to enable it in the configuration file).

- `unixLikeMouseWheel`: if true, using the mouse wheel will take effect directly in the window that is pointed to by the cursor, as opposed the window which currently has the focus. This behavior is standard on Mac and most Linux based DEs.
	-> Uses `horizontalScrollFactor` (you can use this to increase the number of columns scrolled by using the horizontal scroll wheel).
- `smoothVolumeControl`: modifies the handling of the volume, replacing by a logarithmic scale. This means that unlike the linear scale, where a change from 50% to 100% is perceived the same as from 1% to 2% by your ear, a logarithmic scale will prove more handy when quickly adjusting the volume.
	- Uses `volumeIncrementQuantity`.
- `useSoftMediaKeys`: adds special hotkeys for system media keys. Useful on keyboards having no media keys. Ctrl+Win+Home: stop, End: play/pause, PgUp: previous, PgDn: next, Up: vol+, Down: vol-. This is recommended to be left active in all cases as it will provide an alternative if your manufacturer uses nonstandard volume keys, or handles them in a way that cannot be caught (Macs are the only ones I know of currently).
- `ddcCiBrightnessControl`: enables controlling the brightness of your screen through Ctrl+Win+Left and Right. It can be used either for your internal panel (possibly replacing the manufacturer tool if it proves to be badly designed or a resource hog like often), either for your external screen. The screen affected by this command is the one on which the active window is. Note that this won't work on a TV since the DDC/CI protocol is not supported. It will work on most monitors though, but you might have to enable the DDC/CI option somewhere from the OSD to enable for that. Try it if the brightness is stuck to 0.
	- Shift can be held to make smaller increments.
	- Driven by `brightnessIncrementQuantity`, `forceReapplyGammaOnBrightnessChange`.
- `toggleHideFolders`: press Ctrl+H from an explorer Window (note: currently not the desktop or an open file dialog opened from an app) to quickly toggle the display of hidden and system files and folders.
- `rightCtrlContextMenu`: a brief press on the right Control key triggers a virtual keypress on the "context menu" key, which is present on desktop keyboards yet absent from most laptops.
- `startScreenSaverWithInsert`: turns this most usless key into something that may as well be less useful, it launches the screensaver and locks the workstation. This can be useful in strict environments where you want to use your screensaver as a workstation lock with high reliability (since even by moving the mouse a second after the screensaver has started, the password will be asked).
- `reloadConfigWithCtrlWinR`: frankly, leave this to true. Allows a quick relaunch of the utility with the same privileges as it was originally given (that means administrator as well) by pressing Ctrl+Win+R.
- `multiDesktopLikeApplicationSwitcher`: use Ctrl+[numpad 0] to [numpad 9] to bring up the last active window of the task of the same number in the superbar. Actually does what the system already provides with Ctrl+Win+[0-9] but few people know how great it is. If the given application is already the active one, will simply cycle between its windows in the opposite order of last activation. With a bit of habit, it can be very useful as an alternative to multi desktops (and no comment about the fact that Windows 10 introduced them!), if you consider each application as a workspace. You then just have to memorize which app you want to access and press the corresponding number. The last window is usually what you want. Should it not be, just press repeatedly to cycle between windows.
- `winFOpensYourFiles`: press Win+F to open your home folder, just like Win+E for This PC.
- `winHHidesWindow`: press Win+H to minimize the current window. Does the same as Win+Down with the extra "restore size" step if the window was maximized.
- `iAmAMac`: various useful stuff if you have a Mac, especially portable.
	- If `ddcCiBrightnessControl` is set to true and the target panel is the internal LCD, applies a non linear curve to compensate for the poor driver. With this setting, changing from 10% to 15% will have the same visual effect as changing from 90% to 95%, unlike with the default Bootcamp implementation.
	- Eats accidental key presses due to poor hardware handling of the Fn key (this bug has been there for years and is still present). For instance, if you press Fn+Right (=End) and you release Fn slightly before you release the right key, an additional Right key press will be registered, putting your cursor not at the end of the line but the beginning of the next one. When typing quickly, this can be a hassle. The same happens with all other keys (Delete vs Backspace, etc.). Your text editing skills should double after having enabled this option.
- `rightShiftContextMenu`: A quick press on the right shift key will trigger a context menu if rightCtrlContextMenu is true.
- `nonoNumPad`: Left Win+[0-9] do the same as Ctrl+[numpad 0-9] if multiDesktopLikeApplicationSwitcher is true.
- `useCustomGammaCurve`: allows to apply a custom gamma curve. This was used for my HP 8540w which had a pretty good screen in terms of gamut and default white level, but a very poor gamma curve, making everything look bluish. The gamma curve should be extracted from an ICC profile but I've done it by hand so there is no help on this. You shouldn't touch this and rely on Windows' color management (and you are not satisfied, you might want to dig into this functionality).
	- Driven by `autoApplyGammaCurveDelay`, `customGammaCurveGamma`, `customGammaCurveArray`.



Using the Roxxor Tool
---------------------

The Roxxor tool will usually find its way at ease on your system in three steps.

1. Unzipping it somewhere. Run it with the default configuration and try out.
2. Tweaking the config.json file (make sure it stays side by side with the executable). Enable, disable or customize the functionality according to your needs. Relaunch the executable to test it with your parameters. If reloadConfigWithCtrlWinR is true, you can do so simply by pressing Ctrl+Win+R. The program will relaunch itself with the updated configuration.
3. Once you are satisfied and like this utility, you may want to allow it run as administrator at startup. Loading the Roxxor Tool as administrator allows to capture hotkeys from within Administrator windows, such as the Task Manager, and ensure a smooth and consistent experience. This is done by putting the executable some place safe and shared, then setting up a scheduled task.

In order to do this, open the Task Scheduler (Win+R, taskschd.msc, RETURN). Create a new task (recommended at root "Task Scheduler Library") with the following parameters:
- Under the General tab, Choose "Run only when user is logged on"
- Tick "Run with highest privileges"
- Under the "Triggers" tab, add a new Trigger. Set it to begin "At log on". Do not tick anything else than "Enabled".
- Under the "Actions" tab, add a new action that is "Start a program". Click on "Browse" and select the location where you put the tool. Do not move it afterwards. Under "Start in", put the enclosing directory in which the program is located.
- Under the "Conditions" tab, do not tick anything. We want the task to run everywhere, indefinitely (you'll always be able to stop it if you need to, and killing the program from the Task Manager will do the job as well).
- Under the "Settings" tab, tick "Allow the task to be run on demand", "Run task as soon as possible after a scheduled start is missed", "If the running task does not end when requested, force it to stop" and under "If the task is already running, then the following rule applies" set "Stop the existing instance".
- Click OK. You can start and stop the task by right clicking it and selecting "Run". The status should change to "Running" (you may need to refresh the list).
