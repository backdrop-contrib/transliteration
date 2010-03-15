<?php
// $Id$

/**
 * @file
 * Retroactive transliteration and admin settings UI.
 */

/**
 * Form builder function; generates retroactive transliteration confirm form.
 *
 * @see transliteration_retroactive_submit()
 * @ingroup forms
 */
function transliteration_retroactive() {
  if (!$query = transliteration_file_query()) {
    drupal_set_message(t('Database not supported.'), 'error');
    $form['description']['#markup'] = t('Retroactive transliteration is not supported for the database system of this Drupal installation. If you think this should be fixed please <a href="@issues-url">file an issue</a> in the project issue queue.', array('@issues-url' => 'http://drupal.org/project/issues/transliteration'));
    return $form;
  }

  $count = $query->countQuery()->execute()->fetchColumn();
  if (!$count) {
    drupal_set_message(t('Transliteration is not required.'), 'status');
    $form['description']['#markup'] = t('There are currently no files names containing non-ASCII characters.');
    return $form;
  }

  $form['#redirect'] = 'admin/config/media/file-system/settings';
  $question = t('Are you sure you want to transliterate existing file names?');

  // Generate a sample list.
  $items = array();
  foreach($query->range(0, 10)->execute() as $file) {
    $filename = basename($file->uri);
    $items[] = t('!original becomes %renamed', array('!original' => l($filename, file_create_url($file->uri)), '%renamed' => transliteration_clean_filename($filename)));
  }
  $description = '<p><strong>' . t('The database currently lists @x_filenames containing non-ASCII characters.', array('@x_filenames' => format_plural($count, '1 file name', '@count file names'))) . '</strong> ' . t('This count might be inaccurate, though, since some files may not need to be renamed. For example, off-site files will never be changed.') . '</p>';
  $description .= '<p>' . theme('item_list', array('items' => $items)) . ($count > 10 ? t('(Note: only the first 10 entries are shown.)') : '') . '</p>';
  $description .= '<p>' . t('<strong>Warning:</strong> if there are manually inserted links in contents, renaming these files will break them. Since there is currently no automated way to also fix the links, it is a good idea to backup the database and files directory beforehand.') . '</p>';
  $description .= '<p>' . t('This action cannot be undone.') . '</p>';

  return confirm_form($form, $question, 'admin/config/media/file-system/settings', $description, t('Transliterate'));
}

/**
 * Form submit function; retroactively transliterates existing file names.
 *
 * @see transliteration_retroactive()
 */
function transliteration_retroactive_submit($form, &$form_state) {
  $count = 0;
  $errors = array();
  $result = transliteration_file_query()->execute();

  while ($file = $result->fetchObject()) {
    $wrapper = file_stream_wrapper_get_instance_by_uri($file->uri);
    if (!$wrapper) {
      // Missing implementation.
      $errors[] = file_uri_target($file->uri);
      continue;
    }
    if (!in_array('DrupalLocalStreamWrapper', class_parents($wrapper))) {
      // Skip non-local files.
      continue;
    }
    if (!file_exists($wrapper->realpath())) {
      // Missing file.
      $errors[] = file_uri_target($file->uri);
      continue;
    }

    // Sanitize file name.
    $filename = transliteration_clean_filename(basename($file->uri));
    // Build destination URI.
    $destination = file_stream_wrapper_uri_normalize(drupal_dirname($file->uri) . '/' . $filename);
    $destination = file_destination($destination, FILE_EXISTS_RENAME);

    // Rename and update the file record accordingly.
    if ($wrapper->rename($file->uri, $destination)) {
      $file->uri = $destination;
      // Don't use file_save() as it modifies the timestamp.
      drupal_write_record('file', $file, 'fid');
      // Inform modules that the file has been updated.
      module_invoke_all('file_update', $file);
      $count++;
    }
    else {
      $errors[] = file_uri_target($file->uri);
    }
  }

  if ($errors) {
    $message = t('Not all file names could be converted. The following files could not be accessed and have been ignored:');
    $message .= theme('item_list', array('items' => $errors));
    drupal_set_message($message, 'error');
  }
  else {
    drupal_set_message(t('@filenames have been successfully transliterated.', array('@filenames' => format_plural($count, '1 file name', '@count file names'))));
  }

  // Flush page cache.
  cache_clear_all();
}

/**
 * Builds a query that returns all file names from the database containing non-ASCII characters.
 *
 * @return
 *   SelectQuery
 */
function transliteration_file_query() {
  // Regular expressions are not supported by Drupal's database layer and
  // operators differ between manufacturers.
  switch (db_driver()) {
    case 'mysql':
    case 'mysqli':
      $operator = 'NOT REGEXP';
      $regex = '/[a-z0-9_.-]+$';
      break;

    case 'pgsql':
      $operator = '!~*';
      $regex = '/[a-z0-9_.-]+$';
      break;

    default:
      return FALSE;
  }

  return db_select('file')
    ->fields('file')
    ->condition('uri', $regex, $operator);
}

