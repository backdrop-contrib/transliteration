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
    $form['description']['#markup'] = t('Retroactive transliteration is currently not supported for the database system of this Drupal installation. If you think this should be fixed please <a href="@issues-url">file an issue</a> in the project issue queue.', array('@issues-url' => 'http://drupal.org/project/issues/transliteration'));
    return $form;
  }

  $count = $query->countQuery()->execute()->fetchColumn();
  if (!$count) {
    drupal_set_message(t('Transliteration is not required.'), 'status');
    $form['description']['#markup'] = t('There are currently no files names containing non-ASCII characters.');
    return $form;
  }

  $form['#redirect'] = 'admin/config/media/file-system/transliteration';
  $question = t('Are you sure you want to transliterate existing file names?');
  $description = t('The database currently lists @x_filenames containing non-ASCII characters. This count might be inaccurate, though, since it is possible that not all of them actually need to be renamed. Only local files will be processed, while off-site files will never be changed. Note that changing the file name might render references to these files void. Since there is currently no way to fix manually inserted links in contents, it is a good idea to create a database backup beforehand. This action cannot be undone.', array('@x_filenames' => format_plural($count, '1 file name', '@count file names')));
  return confirm_form($form, $question, '<front>', $description, t('Transliterate'));
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

