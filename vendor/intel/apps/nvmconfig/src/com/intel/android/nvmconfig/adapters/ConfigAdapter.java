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

package com.intel.android.nvmconfig.adapters;

import java.util.List;

import com.intel.android.nvmconfig.R;
import com.intel.android.nvmconfig.models.config.Config;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.TextView;

public class ConfigAdapter extends BaseAdapter {

    private List<Config> configs = null;
    private LayoutInflater inflater = null;

    public ConfigAdapter(Context context, List<Config> configs) {

        this.inflater = LayoutInflater.from(context);
        this.configs = configs;
    }

    public int getCount() {
        return this.configs.size();
    }

    public Object getItem(int position) {
        return this.configs.get(position);
    }

    public long getItemId(int position) {
        return position;
    }

    public View getView(int position, View convertView, ViewGroup parent) {

        if (convertView == null) {
            convertView = this.inflater.inflate(R.layout.config_item, null);

            ViewHolder holder = new ViewHolder();
            holder.textViewConfigName = (TextView)convertView.findViewById(R.id.textViewConfigName);
            holder.textViewConfigDescription = (TextView)convertView.findViewById(R.id.textViewConfigDescription);
            convertView.setTag(holder);
        }

        final Config config = this.configs.get(position);

        ViewHolder holder = (ViewHolder)convertView.getTag();
        holder.textViewConfigName.setText(config.getName());
        holder.textViewConfigDescription.setText(config.getDescription());

        return convertView;
    }

    private class ViewHolder {
        public TextView textViewConfigName = null;
        public TextView textViewConfigDescription = null;
    }
}
