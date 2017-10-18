# AAWeaponEditor
AA ingame weapon editor

Thanks and praise to Helios for his hard work making kickass hacks. This tool is meant to support editing weapon config files.

Thanks to Lee Thomason for tinyxml2 https://github.com/leethomason/tinyxml2 which I used in the project.

Requires VC++ 2015 redist, https://www.microsoft.com/en-us/download/details.aspx?id=53840

For advanced users/people who know what they are doing (New users skip to bottom):

This release adds support for multiple firemodes per weapon, and 
customizable attributes to edit. The settings.ini has been replaced with
 Settings.xml, looks like this:

Notice there is a big chunk of comments explaining tags before the first real <Attribute field.

````XML
<!-- Root settings opening tag -->
<WeaponEditorSettings>

  <!-- List of attributes to edit, you can add entries here if there is
  another numeric field in a firemode you want to edit -->
  <AttributesToEdit>

    <!-- ** Attribute name is the exact name of the field to edit.
         The type should be either float or int depending on whether
         the field can accept numbers with decimals or not. Most fields
         will be float. **
         
    <Attribute name="ExactNameInWeaponCfg" type="float">
    
      ** The minimum value this field can have. You cannot lower the value
      below this. If you begin editing a weapon and its original value is less
      than the minimum specified here, it will be set to the minimum as its 
      starting value.**
      
      <MinValue>-1</MinValue>
      
      ** The maximum value this field can have. You cannot raise the value
      above this. If you begin editing a weapon and its original value is more
      than the maximum specified here, it will be set to the maximum as its 
      starting value.**
      
      <MaxValue>10</MaxValue>
      
      ** Delta is an unsigned float specifying how much the value should change
      by each time you raise or lower the value. This should be a positive number.**
      
      <Delta>0.01</Delta>
      
      ** KeyRaiseValue is the decimal virtual key code corresponding to the keyboard
      key you want to raise the value on press. You can find a list of these at this
      site http://cherrytree.at/misc/vk.htm, or https://msdn.microsoft.com/en-us/library/windows/desktop/dd375731%28v=vs.85%29.aspx
      or in one of the AA config files. Or just google it. **
      
      <KeyRaiseValue>104</KeyRaiseValue>
      
      ** KeyLowerValue is the decimal virtual key code corresponding to the keyboard
      key you want to lower the value on press. **
      
      <KeyLowerValue>98</KeyLowerValue>
      
      ** AddToWeaponIfMissing can be true or false. If true, it will add the attribute
      to a weapon firemode if it does not already have it, when first edit attempt occurs. The initial value will be
      the MinValue specified above **
      
      <AddToWeaponIfMissing>true</AddToWeaponIfMissing>
      
    </Attribute>-->
    
    
    <Attribute name="BulletDrop" type="float">
      <MinValue>-1</MinValue>
      <MaxValue>10</MaxValue>
      <Delta>0.01</Delta>
      <KeyRaiseValue>104</KeyRaiseValue>
      <KeyLowerValue>98</KeyLowerValue>
      <AddToWeaponIfMissing>true</AddToWeaponIfMissing>
    </Attribute>

    <Attribute name="BulletSpeed" type="float">
      <MinValue>-1</MinValue>
      <MaxValue>7000</MaxValue>
      <!-- How much to change it by, should be positive-->
      <Delta>10</Delta>
      <KeyRaiseValue>102</KeyRaiseValue>
      <KeyLowerValue>100</KeyLowerValue>
      <AddToWeaponIfMissing>true</AddToWeaponIfMissing>
    </Attribute>

    <Attribute name="PitchToAdd" type="float">
      <MinValue>-1</MinValue>
      <MaxValue>5</MaxValue>
      <!-- How much to change it by, should be positive-->
      <Delta>0.025</Delta>
      <KeyRaiseValue>99</KeyRaiseValue>
      <KeyLowerValue>97</KeyLowerValue>
      <AddToWeaponIfMissing>true</AddToWeaponIfMissing>
    </Attribute>

    <Attribute name="YawToAdd" type="float">
      <MinValue>-1</MinValue>
      <MaxValue>5</MaxValue>
      <!-- How much to change it by, should be positive-->
      <Delta>0.025</Delta>
      <KeyRaiseValue>106</KeyRaiseValue>
      <KeyLowerValue>111</KeyLowerValue>
      <AddToWeaponIfMissing>false</AddToWeaponIfMissing>
    </Attribute>
    
  </AttributesToEdit>

  <!-- Virtual key code used to automatically send keypresses to an AA keybind
  'loadweapons newweaps', Numpad 5 by default -->
  <AAKeyBindKey key="101"/>

  <!-- Virtual key code to select next weapon (or firemode), Numpad 9 by default -->
  <NextWeaponKey key="105" />

  <!-- Virtual key code to select prev weapon (or firemode), Numpad 7 by default -->
  <PrevWeaponKey key="103" />
  
  
</WeaponEditorSettings>
````

So if you wanted to edit an attribute in a firemode that isnt in the 
default settings, you can easily add a new Attribute tag to the 
AttributesToEdit node. For instance, BulletPlayerSpeedScale, set to 
numpad plus = raise and numpad minus = lower.

````XML
    <Attribute name="BulletPlayerSpeedScale" type="float">
      <MinValue>-1</MinValue>
      <MaxValue>10</MaxValue>
      <Delta>0.05</Delta>
      <KeyRaiseValue>107</KeyRaiseValue>
      <KeyLowerValue>108</KeyLowerValue>
      <AddToWeaponIfMissing>true</AddToWeaponIfMissing>
    </Attribute>
````

Otherwise the usage is much the same as previous versions. By default...

NUMPAD7 = Previous firemode (or previous weapon, if we are on first firemode of current weapon)
NUMPAD9 = Next firemode (or next weapon, if we are on last firemode of current weapon)

NUMPAD8 = Raise BulletDrop
NUMPAD 2 = Lower BulletDrop

NUMPAD 4 = Lower BulletSpeed
NUMPAD 6 = Raise BulletSpeed

NUMPAD1/3 = Lower/Raise PitchToAdd
NUMPAD /* = Lower/Raise YawToAdd

Ctrl + Q = Quit

And you can change what keys are used in the Settings.xml

Same as before, drop the exe and Settings.xml into the folder 
containing AA config files for that game. Close the hack/game before 
running the editor IF its the first time you are running the editor in 
that AA config folder. It needs to add a keybind and its easier for me 
to say "close the hack/game first" than try and walk people through 
loading keybinds config. This is for first run only, order after that 
doesn't matter.

Note that a weapon attribute, if beyond the min/max specified in 
Settings.xml, will not be changed to min/max in newweaps until you 
attempt to edit that firemode/weapon attribute. In other words, the 
values in newweaps for bulletspeed, drop, etc, will stay the same as 
what was in the original weapon config, until you try to edit that 
specific weapon/firemode bulletspeed or drop or whatever.

The program works by creating an AA keybind in the keybinds config 
file in the folder you dropped it in, by default 
NUMPAD5=OnPress:loadweapons newweaps. The program will create the 
keybind when run, and will send a keystroke which triggers that keybind,
 every time a weapon value is changed.

If you are running the program for the first time in a given AA 
config file folder, you should have the game and hack closed. The 
program needs to create a new keybind in the AA keybind config file, and
 the hack won't notice this new keybind if it is created while the hack 
is running. If you ignore these instructions, type "loadkeybinds" into 
the AA hack console and hit enter.

Again, that is just for the first time you are running the editor in 
that AA config file folder. Once the loadweapons newweaps keybind is 
added, the order doesn't matter, as long as the hack is aware of the 
keybind.

If it doesn't seem to be working, type "showkeybinds" into the AA 
console without quotes and hit enter. Make sure you see the 'loadweapons
 newweaps' keybind at whatever key you set in Settings.xml. By default 
its NUMPAD5=OnPress:loadweapons newweaps

If you try to run the program and it closes right away, it 
encountered an error. Run the program from a command prompt (Windows key
 + R, type cmd & hit enter, navigate to folder, type exe name and 
hit enter) and see what the output is. If the output doesn't help or 
doesn't make sense to you, you might have found a bug, feel free to 
contact me.

If you find a weapon config you like, be sure to back up newweaps as 
it will be overwritten the next time you run the editor. I've added a 
function to back up the last newweaps before overwriting, in case you 
forget or something.

For New Users:

This tool allows you to edit weapon config file while in game.
See the first post in the thread and read that, then read the above to see whats new in the version.

What else has changed?

I rewrote most of this, so its not as giant a mess of sphagetti code.
 I use stl containers to store weapons, firemodes, and attributes and 
such, instead of roll your own data structures. No more memory leaks. 
Instead of using hardcoded attribute names, hardcoded indexes for where 
to find them, etc, I now use regular expressions and generics to 
extract/modify attributes. This means if someone else wants to edit 
this, it will be much easier for them to change. They can just call a 
function GetTaggedAttribute(attrName... to get the value, and an equiv 
setter.

        
