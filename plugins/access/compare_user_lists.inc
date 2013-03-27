<?php

/**
 * @file
 * Ctools access plugin to provide access/visiblity if two user contexts are equal.
 */

/**
 * Plugins are described by creating a $plugin array which will be used
 * by the system that includes this file.
 */
$plugin = array(
  'title' => t("User: compare list"),
  'description' => t('Compare two users or user lists'),
  'callback' => 'acec_compare_user_lists_access_check',
  'default' => array(
    'contains' => 1,
  ),
  'settings form' => 'acec_compare_user_lists_settings',
  'summary' => 'acec_compare_user_lists_ctools_access_summary',
  'required context' => array(
    new ctools_context_required(t('First User'), 'user'),
    new ctools_context_required(t("Second User"), 'user')
  ),
);

/**
 * Settings form for the 'by perm' access plugin
 */
function acec_compare_user_lists_settings($form, &$form_state, $conf) {

  $form['settings']['helptext'] = array(
    '#type' => 'markup',
    '#value' => '<div>' . t('Grant access based on comparison of the two user contexts. For example, to grant access to a user to view their own profile, choose "logged in user" and "user being viewed" and say "grant access if equal". When they\'re the same, access will be granted.') . '</div>',
  );

  $form['settings']['contains'] = array(
    '#type' => 'radios',
    '#title' => t('Grant access if user contexts are'),
    '#options' => array(1 => t('Contains'), 0 => t('Does not contain')),
    '#default_value' => $conf['contains'],
  );
  return $form;
}

/**
 * Check for access.
 */
function acec_compare_user_lists_access_check($conf, $context) {

  if (empty($context) || count($context) != 2 || empty($context[0]->data) || empty($context[1]->data)) {
    return FALSE;
  }

  if (!is_array($context[0]->data)) {
    $context[0]->data = array($context[0]->data->uid => $context[0]->data);
  }
  if (!is_array($context[1]->data)) {
    $context[1]->data = array($context[1]->data->uid => $context[1]->data);
  }

  $accounts1 = array_keys($context[0]->data);
  $accounts2 = array_keys($context[1]->data);

  $access = array_intersect_assoc($accounts1, $accounts2);

  // xor returns false if the two bools are the same, and true if they are not.
  // i.e, if we asked for equality and they are equal, return true.
  // If we asked for inequality and they are equal, return false.
  return (array_intersect_assoc($accounts1, $accounts2)) xor empty($conf['contains']);
}

/**
 * Describe an instance of this plugin.
 */
function acec_compare_user_lists_ctools_access_summary($conf, $context) {
  $comparison = !empty($conf['contains']) ? 'contains' : 'does not contain';

  return t('@id1 @comp @id2', array('@comp' => $comparison, '@id1' => $context[0]->identifier, '@id2' => $context[1]->identifier));
}
