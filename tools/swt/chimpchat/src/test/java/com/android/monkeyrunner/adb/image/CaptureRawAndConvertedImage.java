/*
 * Copyright (C) 2013 The Android Open Source Project
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
 */
package com.android.monkeyrunner.adb.image;

import com.android.chimpchat.adb.image.CaptureRawAndConvertedImage.IRawImager;
import com.android.ddmlib.RawImage;

import java.io.Serializable;

@Deprecated
public class CaptureRawAndConvertedImage {

    /**
     * When com.android.chimpchat.ImageUtilsTest.createRawImage(String) deserializes
     * an image from a file, it might try to recreate this older class -- the class itself
     * is obsolete and replaced by the {@code ChipRawImage} but this exact implementation
     * is still needed for deserialization compatibility purposes.
     * <p/>
     * This implementation is restored from sdk.git @ 8c09f35fe02c38c18f8f7b9e0a531d6ac158476e.
     */
    @Deprecated
    public static class MonkeyRunnerRawImage implements Serializable, IRawImager {
        private static final long serialVersionUID = -1137219979977761746L;
        public int version;
        public int bpp;
        public int size;
        public int width;
        public int height;
        public int red_offset;
        public int red_length;
        public int blue_offset;
        public int blue_length;
        public int green_offset;
        public int green_length;
        public int alpha_offset;
        public int alpha_length;

        public byte[] data;

        public MonkeyRunnerRawImage(RawImage rawImage) {
            version = rawImage.version;
            bpp = rawImage.bpp;
            size = rawImage.size;
            width = rawImage.width;
            height = rawImage.height;
            red_offset = rawImage.red_offset;
            red_length = rawImage.red_length;
            blue_offset = rawImage.blue_offset;
            blue_length = rawImage.blue_length;
            green_offset = rawImage.green_offset;
            green_length = rawImage.green_length;
            alpha_offset = rawImage.alpha_offset;
            alpha_length = rawImage.alpha_length;

            data = rawImage.data;
        }

        @Override
        public RawImage toRawImage() {
            RawImage rawImage = new RawImage();

            rawImage.version = version;
            rawImage.bpp = bpp;
            rawImage.size = size;
            rawImage.width = width;
            rawImage.height = height;
            rawImage.red_offset = red_offset;
            rawImage.red_length = red_length;
            rawImage.blue_offset = blue_offset;
            rawImage.blue_length = blue_length;
            rawImage.green_offset = green_offset;
            rawImage.green_length = green_length;
            rawImage.alpha_offset = alpha_offset;
            rawImage.alpha_length = alpha_length;

            rawImage.data = data;
            return rawImage;
        }
    }

}
