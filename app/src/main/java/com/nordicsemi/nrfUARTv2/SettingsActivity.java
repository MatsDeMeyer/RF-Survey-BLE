package com.nordicsemi.nrfUARTv2;

import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.preference.PreferenceActivity;
import android.preference.PreferenceManager;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;

public class SettingsActivity extends Activity {

    public static final String KEY_PREF_ACCESS_PROFILE = "accessProfilePreference";
    public static final String KEY_PREF_FILE_ID = "fileIDPreference";
    public static final String KEY_PREF_FILE_OFFSET = "offsetPreference";
    public static final String KEY_PREF_FILE_LENGTH = "fileLengthPreference";


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Display the fragment as the main content.
        getFragmentManager().beginTransaction()
                .replace(android.R.id.content, new SettingsFragment())
                .commit();
    }

    public void onBackPressed()
    {
        startActivity(new Intent(SettingsActivity.this, MainActivity.class));
    }
}
