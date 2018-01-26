<?php
/**
 * @file
 * drupal_extensions_status_list
 *
 *
 *  Copyright 2018 Leiden University Library
 *
 *  This file is part of drupal_extensions_status_list.
 *
 *  drupal_extensions_status_list is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/**
 * Implements hook_drush_command().
 */
function drupal_extensions_status_list_drush_command() {
  $items['drupal_extensions_status_list'] = array(
    'description' => 'Outputs a list of all extensions of the current Drupal site and adds information about git status.',
    'options' => array(
      'format' => 'Optional, format of the output string.',
    ),
    'aliases' => array('desl'),
    'examples' => array(
      'drush --user=admin drupal_extensions_status_list',
      'drush --user=admin desl',
    ),
  );
  return $items;
}

/**
 * Implements drush_hook_COMMAND().
 */
function drush_drupal_extensions_status_list() {
  $format = drush_get_option('format', "[modulename];[package];[type];[version];[status];[dirname];[gitcommit];[gitbranch];[gitstatus]\n");
  $pmlist = system_rebuild_module_data();

  drush_shell_exec("drush drupal-directory");
  $drupaldirs = drush_shell_exec_output();
  $drupaldir = end($drupaldirs);
   
  $result = array();

  foreach ($pmlist as $extname => $extinfo) {
    $desl = array(
      '[modulename]' => $extname,
      '[filename]' => $extinfo->filename,
      '[dirname]' => dirname($extinfo->filename),
      '[name]' => $extinfo->info['name'],
      '[description]' => $extinfo->info['description'],
      '[package]' => $extinfo->info['package'],
      '[core]' => $extinfo->info['core'],
      '[version]' => $extinfo->info['version'],
      '[project]' => $extinfo->info['project'],
      '[type]' => $extinfo->type,
      '[status]' => ($extinfo->status?'enabled':'disabled'),
    );

    $directory = $drupaldir . '/' . $desl['[dirname]'];

    if (file_exists("$directory/.git")) {
      if (drush_shell_exec("(cd '$directory' && git status --porcelain)")) {
        $output = drush_shell_exec_output();
        $desl['[gitstatus]'] = implode(',', $output);
      }
      else {
        $desl['[gitstatus]'] = '';
      }
      if (drush_shell_exec("(cd '$directory' && git rev-parse --abbrev-ref HEAD)")) {
        $output = drush_shell_exec_output();
        $desl['[gitbranch]'] = implode(',', $output); 
      }
      if (drush_shell_exec("(cd '$directory' && git log --oneline -1)")) {
        $output = drush_shell_exec_output();
        $lastcommit = count($output)?$output[0]:'';
        list($hash, $desc) = explode(' ', $lastcommit, 2);
        if (drush_shell_exec("(cd '$directory' && git tag --contains $hash)")) {
          $output = drush_shell_exec_output();
          if (count($output)) {
            $hash = $hash . ' (' . implode(',', $output) . ')';
          }
        }
        $desl['[gitcommit]'] = $hash; 
      }
    }
    else {
      $desl['[gitcommit]'] = 'no git';
      $desl['[gitbranch]'] = 'no git';
      $desl['[gitstatus]'] = 'no git';
    }
    $line = str_replace(array_keys($desl), array_values($desl), $format);
    
    $result[$desl['[modulename]']] = $line;
  }
  
  uksort($result, "strcasecmp");
  foreach ($result as $line) {
    print($line);
  }
}
