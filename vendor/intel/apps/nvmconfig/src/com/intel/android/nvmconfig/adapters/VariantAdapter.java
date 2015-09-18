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
import com.intel.android.nvmconfig.models.config.Variant;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.TextView;

public class VariantAdapter extends BaseAdapter {

    private List<Variant> variants = null;
    private LayoutInflater inflater = null;

    public VariantAdapter(Context context, List<Variant> variants) {

        this.inflater = LayoutInflater.from(context);
        this.variants = variants;
    }

    public int getCount() {
        return this.variants.size();
    }

    public Object getItem(int position) {
        return this.variants.get(position);
    }

    public long getItemId(int position) {
        return position;
    }

    public View getView(int position, View convertView, ViewGroup parent) {

        if (convertView == null) {
            convertView = this.inflater.inflate(R.layout.variant_item, null);

            ViewHolder holder = new ViewHolder();
            holder.textViewVariantName = (TextView)convertView.findViewById(R.id.textViewVariantName);
            holder.textViewVariantCreator = (TextView)convertView.findViewById(R.id.textViewVariantCreator);
            convertView.setTag(holder);
        }

        final Variant variant = this.variants.get(position);

        if (variant != null) {
            ViewHolder holder = (ViewHolder)convertView.getTag();
            holder.textViewVariantName.setText(variant.getName());
            holder.textViewVariantCreator.setText(variant.getCreationDate());
        }

        return convertView;
    }

    private class ViewHolder {
        public TextView textViewVariantName = null;
        public TextView textViewVariantCreator = null;
    }
}
