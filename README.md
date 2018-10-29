# The Roxxor Tool

~Many small tools to improve your workflow on Windows.~

I decided to publicly release a tool that I've been developing and improving ever since I have got a computer. I have a little notstalgic love for Windows, I know it isn't common, but anyway. And let's admit it, this OS is far from perfect in its default configuration. But it is fairly easy to implement the things missing by yourself.

This tool focuses on very basic functionality that is usually not covered by other tools (else I would simply have used them in the first place). So let's go on with a list of tweaks, associated with their key (which you'll use to enable it in the configuration file).

- `scrollAccelerationFactor`: set to a value > 0 (start with 0.5) to enable mouse wheel acceleration, a bit like on macOS.
	- `scrollAccelerationIntertia`: this sets the scroll rate necessary to accelerate the scrolling. The smaller the value, the less you'll see acceleration, and the quicker it will stop once you release it.
	- `scrollAccelerationMaxScrollFactor`: limits the maximum scrolling rate.
	- `scrollAccelerationSendMultipleMessages`: by default it uses the precise scrolling as offered by Windows, but you may set this to true to simulate an accelerated scroll by multiple simple scroll actions.
	- `scrollAccelerationDismissTrackpad`: when set to true, dismisses any "precise" device (i.e. not having a fixed wheel delta, like gaming mice). Try setting this to false if it doesn't work.
- `ddcCiBrightnessControl`: enables controlling the brightness of your screen through Ctrl+Win+F9 and F10. It can be used either for your internal panel (possibly replacing the manufacturer tool if it proves to be badly designed or a resource hog like often), either for your external screen. The screen affected by this command is the one on which the active window is. Note that this won't work on a TV since the DDC/CI protocol is not supported. It will work on most monitors though, but you might have to enable the DDC/CI option somewhere from the OSD to enable for that. Try it if the brightness is stuck to 0.
	- Shift can be held to make smaller increments.
	- Driven by `brightnessIncrementQuantity`, `forceReapplyGammaOnBrightnessChange`, `brightnessCacheDuration` (time in ms in which the brightness is kept in memory before querying again the panel's current brightness; this is useful if you use the adaptive display feature -- try setting it to 0 to see what I mean -- else I recommend you turn it to 0).
	- If `wmiLogarithmicBrightness` is set to true (and `ddcCiBrightnessControl` active), and the target panel is the internal LCD (i.e. your mouse/active app is on this monitor when you press the shortcut), applies a non linear curve to compensate for the poor driver. With this setting, changing from 10% to 15% will have the same visual effect as changing from 90% to 95%, unlike with the default Bootcamp implementation. As implied, mostly suited to Macs.
	- Set `allowNegativeBrightness` to true (default) to allow setting brightness until -40%. This further darkens the screen, very useful on screens that still are too bright on minimum setting, but may be dangerous if the roxxortool is killed improperly, because it won't be able to restore the screen, and next time it will take that darkened screen as a standard color profile. A reboot will fix it in this case.
- `smoothVolumeControl`: modifies the handling of the volume, replacing by a logarithmic scale. This means that unlike the linear scale, where a change from 50% to 100% is perceived the same as from 1% to 2% by your ear, a logarithmic scale will prove more handy when quickly adjusting the volume.
	- Uses `volumeIncrementQuantity`.
- `useSoftMediaKeys`: adds special hotkeys for system media keys. Useful on keyboards having no media keys. Ctrl+Win+Home: stop, End: play/pause, PgUp: previous, PgDn: next, F11: vol-, F12: vol+. This is recommended to be left active in all cases as it will provide an alternative if your manufacturer uses nonstandard volume keys, or handles them in a way that cannot be caught (Macs are the only ones I know of currently).
- `multiDesktopLikeApplicationSwitcher`: use Ctrl+[numpad 0] to [numpad 9] to bring up the last active window of the task of the same number in the superbar. Actually does what the system already provides with Ctrl+Win+[0-9] but few people know how great it is. If the given application is already the active one, will simply cycle between its windows in the opposite order of last activation. With a bit of habit, it can be very useful as an alternative to multi-desktops (and no comment about the fact that Windows 10 introduced them!), if you consider each application as a workspace. You then just have to memorize which app you want to access and press the corresponding number. The last window is usually what you want. Should it not be, just press repeatedly to cycle between windows.
	-> Uses `noNumPad` (if set to true you may also use Win+[0-9] to perform Ctrl+Win+[0-9], but then you have no alternative to do the basic Win+[0-9] done by the system; I still use that on my laptops as they have no numpad and I find the Win+[0-9] too unpredictable; should I really want to cycle against windows I'll use Win+T -- see `winTSelectsLastTask` -- and move with arrow keys to highlight the task and then press Up to navigate).
- `reloadConfigWithCtrlWinR`: frankly, leave this to true. If you press Ctrl+Win+R, the configuration file will be opened with Notepad so you can freely modify the parameters, and once you close it the utility will be relaunched with the same privileges as it was originally given (that means administrator as well).
	-> Warning: until the time the editor (notepad) is launched, the RoxxorTool is inactive (no shortcuts, options etc. work).
	-> Also note that enabling this flag will also enable Ctrl+Win+D, which shows debug info (1 second after the combination has been pressed).
- `iAmAMac`: various useful stuff if you have a Mac, especially portable.
	- Eats accidental key presses due to poor hardware handling of the Fn key (this bug has been there for years and is still present). For instance, if you press Fn+Right (=End) and you release Fn slightly before you release the right key, an additional Right key press will be registered, putting your cursor not at the end of the line but the beginning of the next one. When typing quickly, this can be a hassle. The same happens with all other keys (Delete vs Backspace, etc.). Your text editing skills should double after having enabled this option.
	- If using this administrator rights are recommended since the tool may easily get confused in case the system doesn't forward some events.
- `rightShiftContextMenu`: A quick press on the right shift key without other key in the meantime will trigger a virtual keypress on the "context menu" key, which is present on desktop keyboards yet absent from some laptops.
- `winFOpensYourFiles`: press Win+F to open your home folder, just like Win+E for This PC.
- `winHHidesWindow`: press Win+H to minimize the current window. Does the same as Win+Down with the extra "restore size" step if the window was maximized.
- `closeWindowWithWinQ`: press Win+Q to issue an Alt+F4 (better explained this way ^^), basically closing the active window when allowed by the app. Much more comfortable combination to use, especially after coming back from Mac (note though that this only closes the current window, not the full app, but in most cases it's still closer to Mac's Win+Q than Win+W).
- `winTSelectsLastTask`: press Win+T to switch focus to the taskbar, selecting the LAST task in the list (note: the system does it already but selects the FIRST one instead, or sometimes doesn't work on the first press). Since my taskbar has a lot of pinned shortcuts and the last ones are not accessible through Win+[0-9], this is a poor man's solution. Once you pressed Win+T, you may then navigate using the following keys from the alphanumeric pad:
	- 5: select 11th item (first one inaccessible with Win+[0-9])
	- 6: select 21st item
	- 7: select 31st item
	- Note that Home and End are provided by the system to move to the first or last task. You may as well use a directional key to wrap around the last entry and select the first entry easily (for example if your taskbar is at the bottom, press Right once).
	- You can use PgUp and PgDn keys to move by increments of `winTTaskMoveBy` (default 4) icons in the taskbar.
- `disableWinTabAnimation`: when pressing Win+Tab (and not using another combination, such as swiping up with three/four fingers on your precision touchpad) the Task View appears without animation. Huge hack as it is, it actually removes animations then calls the Task View screen, then periodically checks that it is still active; when it's not it reenables animations. Note that this is done in a safe way, i.e. not persisted to your user profile.
- `altTabWithMouseButtons`: press the forward button on your mouse to open the app selector. Press for longer (200ms or more) to do a standard forward command.
- `selectHiraganaByDefault`: if set, when switching between languages with Win+Space, sends a Ctrl+Caps right behind so that hiragana mode is enabled if IME is japanese (for others doesn't do anything; for other complex IME you may not want to use this feature as it's tailored for Japanese and doesn't have a way to check what you're currently using).
- `winSSuspendsSystem`: suspends Windows by pressing Win+S.
- `doNotUseWinSpace`: replaces Win+Space by Alt+Shift, the shortcut from older Windows versions. The advantage of Alt+Shift is that it switches only between the two last languages, where Win+Space goes to the next on the list, which is often unwanted especially since Windows sometimes temporarily adds unrelated languages during updates until a reboot.
- `internationalUsKeyboardForFrench`: enables easy access to latin-international characters on top of an English (US) keyboard. Aimed at coders, and in its current implementation for French-speakers by providing a couple of shortcuts for mostly used accented characters. Useful since the MS KLC is not really supported anymore and doesn't create touch keyboards. It does this:
	- Right Alt + E = � (or � with Shift when pressing E)
	- Right Alt + C = �
	- Right Alt + A = �
	- Right Alt + U = ` (dead key; then press another key like A to get �, E to get �, etc. or space to insert the ` char)
	- Right Alt + I = ^ (dead key; A=�, E=�, I=�, O=�, U=�, space=^)
	- Right Alt + O = � (dead key; A=�, E=�, I=�, O=�, U=�, space=�)
	- Right Alt + P = � (dead key; A=�, E=�, I=�, O=�, U=�, space=�)
	- Right Alt + N = ~ (dead key; N=�, space=~)
	- Right Alt + ; (semi-colon) = � (horizontal ellipsis)
	- Note that you may create your own layouts or customize it. Look at layoutTranslatorsRegister.

Additional (not as good, please shout out if you use them else I might remove them in the future):

- `toggleHideFolders`: press Ctrl+H from an explorer Window (note: currently not the desktop or an open file dialog opened from an app) to quickly toggle the display of hidden and system files and folders. You need to refresh the desktop afterwards. Other windows get refreshed immediately. Note that sometimes after a reboot you may need to press it.
- `startScreenSaverWithInsert`: turns this most usless key into something that may as well be less useful: it launches the screensaver and locks the workstation. This can be useful in strict environments where you want to use your screensaver as a workstation lock with high reliability (since even by moving the mouse a second after the screensaver has started, the password will be asked). I have implemented this after getting used to the Mac way (no lock, use a screensaver instead, and I had it assigned to Insert through the venerable Karabiner when the system APIs still allowed it), but since then Windows 10 I like the spotlight screen more. I think this feature doesn't work properly anymore (and locks the machine instead).
- `altGraveToStickyAltTab`: frankly this shouldn't be there, it only works on European and American keyboards, but yeah, you may use the Alt key + grave accent (which should be located right above the Tab key, on the leftmost corner) to bring the Alt+Tab menu and keep it open until you click elsewhere or press Escape. There is no magic behind this as the system already does that with Ctrl+Alt+Tab, but since I'm using it a lot I wanted to give it a dedicated key. Funnily I now have it assigned to a mouse button.
	-> I recommend tuning the options of your Alt+Tab view, especially if you've been used to non-Windows DEs. I make the icons bigger by tuning the registry under `HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\MultitaskingView\AltTabViewHost`, setting `Thumbnail_icon_size` to 00000030(hex), `Thumbnail_max_height_percent` to 00000010 and `Thumbnail_min_height_percent` to 00000010. See [this topic](https://answers.microsoft.com/en-us/insider/forum/insider_wintp-insider_personal/registry-customization-for-multitaskingview-mtv/806a0c84-b203-46fb-b23a-acc82dc0ecce) for more info.
- `resetDefaultGammaCurve`: does not take the current gamma curve in account when lowering the brightness below 0%.



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
