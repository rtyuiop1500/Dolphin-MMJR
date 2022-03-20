// SPDX-License-Identifier: GPL-2.0-or-later

package org.dolphinemu.dolphinemu.features.settings.model;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

public enum FloatSetting implements AbstractFloatSetting
{
  // These entries have the same names and order as in C++, just for consistency.

  GFX_FONT_SCALE(Settings.FILE_GFX, Settings.SECTION_GFX_SETTINGS, "FontScale", 2.50f),
  GFX_DISPLAY_SCALE(Settings.FILE_GFX, Settings.SECTION_GFX_SETTINGS, "DisplayScale", 1.0f),
  MAIN_EMULATION_SPEED(Settings.FILE_DOLPHIN, Settings.SECTION_INI_CORE, "EmulationSpeed", 1.0f),
  MAIN_OVERCLOCK(Settings.FILE_DOLPHIN, Settings.SECTION_INI_CORE, "Overclock", 1.0f);

  private static final FloatSetting[] NOT_RUNTIME_EDITABLE_ARRAY = new FloatSetting[]{
          GFX_FONT_SCALE
  };

  private static final Set<FloatSetting> NOT_RUNTIME_EDITABLE =
          new HashSet<>(Arrays.asList(NOT_RUNTIME_EDITABLE_ARRAY));

  private final String mFile;
  private final String mSection;
  private final String mKey;
  private final float mDefaultValue;

  FloatSetting(String file, String section, String key, float defaultValue)
  {
    mFile = file;
    mSection = section;
    mKey = key;
    mDefaultValue = defaultValue;
  }

  @Override
  public boolean isOverridden(Settings settings)
  {
    if (settings.isGameSpecific() && !NativeConfig.isSettingSaveable(mFile, mSection, mKey))
      return settings.getSection(mFile, mSection).exists(mKey);
    else
      return NativeConfig.isOverridden(mFile, mSection, mKey);
  }

  @Override
  public boolean isRuntimeEditable()
  {
    for (FloatSetting setting : NOT_RUNTIME_EDITABLE)
    {
      if (setting == this)
        return false;
    }

    return NativeConfig.isSettingSaveable(mFile, mSection, mKey);
  }

  @Override
  public boolean delete(Settings settings)
  {
    if (NativeConfig.isSettingSaveable(mFile, mSection, mKey))
    {
      return NativeConfig.deleteKey(settings.getWriteLayer(), mFile, mSection, mKey);
    }
    else
    {
      return settings.getSection(mFile, mSection).delete(mKey);
    }
  }

  @Override
  public float getFloat(Settings settings)
  {
    if (NativeConfig.isSettingSaveable(mFile, mSection, mKey))
    {
      return NativeConfig.getFloat(NativeConfig.LAYER_ACTIVE, mFile, mSection, mKey, mDefaultValue);
    }
    else
    {
      return settings.getSection(mFile, mSection).getFloat(mKey, mDefaultValue);
    }
  }

  @Override
  public void setFloat(Settings settings, float newValue)
  {
    if (NativeConfig.isSettingSaveable(mFile, mSection, mKey))
    {
      NativeConfig.setFloat(settings.getWriteLayer(), mFile, mSection, mKey, newValue);
    }
    else
    {
      settings.getSection(mFile, mSection).setFloat(mKey, newValue);
    }
  }

  public float getFloatGlobal()
  {
    return NativeConfig.getFloat(NativeConfig.LAYER_ACTIVE, mFile, mSection, mKey, mDefaultValue);
  }

  public void setFloatGlobal(int layer, float newValue)
  {
    NativeConfig.setFloat(layer, mFile, mSection, mKey, newValue);
  }
}
