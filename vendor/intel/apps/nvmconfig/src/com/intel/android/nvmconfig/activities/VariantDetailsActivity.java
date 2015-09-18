/* Android Modem NVM Configuration Tool
 *
 * Copyright (C) Intel 2012
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: Edward Marmounier <edwardx.marmounier@intel.com>
 */

package com.intel.android.nvmconfig.activities;

import com.intel.android.nvmconfig.R;
import com.intel.android.nvmconfig.adapters.ConfigAdapter;
import com.intel.android.nvmconfig.models.config.Variant;

import android.app.Activity;
import android.os.Bundle;
import android.view.Window;
import android.widget.ListView;
import android.widget.TextView;

public class VariantDetailsActivity extends Activity {

    private TextView textViewVariantName = null;
    private TextView textViewVariantDescription = null;
    private ListView listViewConfigs = null;

    private Variant variant = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);

        super.requestWindowFeature(Window.FEATURE_CUSTOM_TITLE);

        super.setContentView(R.layout.variant_details);

        if (super.getWindow() != null) {
            super.getWindow().setFeatureInt(Window.FEATURE_CUSTOM_TITLE, R.layout.title_bar);
        }

        this.textViewVariantName = (TextView)super.findViewById(R.id.textViewVariantName);
        this.textViewVariantDescription = (TextView)super.findViewById(R.id.textViewVariantDescription);
        this.listViewConfigs = (ListView)super.findViewById(R.id.listViewConfigs);

        this.variant = (Variant)super.getIntent().getSerializableExtra(Intents.EXTRA_VARIANT);

        if (this.variant != null) {
            this.updateUi();
        }
        else {
            super.finish();
        }
    }

    private void updateUi() {

        if (this.textViewVariantName != null) {
            this.textViewVariantName.setText(this.variant.getName());
        }
        if (this.textViewVariantDescription != null) {
            this.textViewVariantDescription.setText(this.variant.getDescription());
        }
        if (this.listViewConfigs != null) {
            this.listViewConfigs.setAdapter(new ConfigAdapter(this, this.variant.getConfigs()));
        }
    }
}
