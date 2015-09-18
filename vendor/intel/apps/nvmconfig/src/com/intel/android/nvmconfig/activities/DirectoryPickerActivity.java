/*THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.*/

package com.intel.android.nvmconfig.activities;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;

import com.intel.android.nvmconfig.R;

import android.app.ListActivity;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Environment;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ListView;
import android.widget.Toast;

// this class was copied from http://bgreco.net/directorypicker/
public class DirectoryPickerActivity extends ListActivity {

    public static final String START_DIR = "startDir";
    public static final String ONLY_DIRS = "onlyDirs";
    public static final String SHOW_HIDDEN = "showHidden";
    public static final String CHOSEN_DIRECTORY = "chosenDir";
    public static final int PICK_DIRECTORY = 43522432;
    private File dir;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Bundle extras = getIntent().getExtras();

        if (extras == null) {
            return;
        }

        dir = Environment.getExternalStorageDirectory();

        if (dir == null) {
            return;
        }

        String preferredStartDir = extras.getString(START_DIR);
        final boolean showHidden = extras.getBoolean(SHOW_HIDDEN, false);
        final boolean onlyDirs = extras.getBoolean(ONLY_DIRS, true);
        if (preferredStartDir != null) {
            File startDir = new File(preferredStartDir);
            if (startDir.isDirectory()) {
                dir = startDir;
            }
        }

        setContentView(R.layout.chooser_list);
        setTitle(dir.getAbsolutePath());
        Button btnChoose = (Button) findViewById(R.id.btnChoose);
        String name = dir.getName();
        if (name.length() == 0) {
            name = "/";
        }

        if (btnChoose != null) {
            btnChoose.setText("Choose " + "'" + name + "'");
            btnChoose.setOnClickListener(new View.OnClickListener() {
                public void onClick(View v) {
                    returnDir(dir.getAbsolutePath());
                }
            });
        }

        ListView lv = getListView();

        if (lv != null) {
            lv.setTextFilterEnabled(true);
        }

        if (!dir.canRead()) {
            Context context = getApplicationContext();
            String msg = "Could not read folder contents.";
            Toast toast = Toast.makeText(context, msg, Toast.LENGTH_LONG);
            toast.show();
            return;
        }

        final ArrayList<File> files = filter(dir.listFiles(), onlyDirs, showHidden);
        String[] names = names(files);
        setListAdapter(new ArrayAdapter<String>(this, R.layout.list_item, names));

        if (lv != null) {
            lv.setOnItemClickListener(new OnItemClickListener() {
                public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                    if (!files.get(position).isDirectory()) {
                        return;
                    }
                    String path = files.get(position).getAbsolutePath();
                    Intent intent = new Intent(DirectoryPickerActivity.this, DirectoryPickerActivity.class);
                    intent.putExtra(DirectoryPickerActivity.START_DIR, path);
                    intent.putExtra(DirectoryPickerActivity.SHOW_HIDDEN, showHidden);
                    intent.putExtra(DirectoryPickerActivity.ONLY_DIRS, onlyDirs);
                    startActivityForResult(intent, PICK_DIRECTORY);
                }
            });
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == PICK_DIRECTORY && resultCode == RESULT_OK) {
            Bundle extras = data.getExtras();

            if (extras != null) {
                String path = (String) extras.get(DirectoryPickerActivity.CHOSEN_DIRECTORY);
                returnDir(path);
            }
        }
    }

    private void returnDir(String path) {
        Intent result = new Intent();
        result.putExtra(CHOSEN_DIRECTORY, path);
        setResult(RESULT_OK, result);
        finish();
    }

    public ArrayList<File> filter(File[] file_list, boolean onlyDirs, boolean showHidden) {
        ArrayList<File> files = new ArrayList<File>();

        if (file_list != null) {
            for (File file: file_list) {
                if (onlyDirs && !file.isDirectory()) {
                    continue;
                }
                if (!showHidden && file.isHidden()) {
                    continue;
                }
                files.add(file);
            }
            Collections.sort(files);
        }
        return files;
    }

    public String[] names(ArrayList<File> files) {
        String[] names = new String[files.size()];
        int i = 0;
        for (File file: files) {
            names[i] = file.getName();
            i++;
        }
        return names;
    }
}

