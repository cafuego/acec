<?php

/**
 * @file
 *
 * Plugin to provide a node context. A node context is a node wrapped in a
 * context object that can be utilized by anything that accepts contexts.
 */

/**
 * Plugins are described by creating a $plugin array which will be used
 * by the system that includes this file.
 */
$plugin = array(
  'title' => t("Entity list"),
  'description' => t('List of entity objects.'),
  'context' => 'acec_context_create_entities',
  'edit form' => 'acec_context_entities_settings_form',
  'defaults' => array('entity_id' => ''),
  'convert list' => 'acec_context_entities_convert_list',
  'convert' => 'acec_context_entities_convert',
  'placeholder form' => array(
    '#type' => 'textfield',
    '#description' => t('Enter the ID of an entity for this context.'),
  ),
  'get child' => 'acec_context_entities_get_child',
  'get children' => 'acec_context_entities_get_children',
);

function acec_context_entities_get_child($plugin, $parent, $child) {
  $plugins = acec_context_entities_get_children($plugin, $parent);
  return $plugins[$parent . ':' . $child];
}

function acec_context_entities_get_children($plugin, $parent) {
  $entities = entity_get_info();
  $plugins = array();
  foreach ($entities as $entity_type => $entity) {
    $child_plugin = $plugin;
    $child_plugin['title'] = $entity['label'];
    $child_plugin['keyword'] = $entity_type;
    $child_plugin['context name'] = $entity_type;
    $child_plugin['name'] = $parent . ':' . $entity_type;
    $child_plugin['description'] = t('Creates @entity context from an entity ID.', array('@entity' => $entity_type));
    $child_plugin_id = $parent . ':' . $entity_type;
    drupal_alter('ctools_entity_context', $child_plugin, $entity, $child_plugin_id);
    $plugins[$child_plugin_id] = $child_plugin;
  }
  drupal_alter('ctools_entity_contexts', $plugins);
  return $plugins;
}

/**
 * It's important to remember that $conf is optional here, because contexts
 * are not always created from the UI.
 */
function acec_context_create_entities($empty, $data = NULL, $conf = FALSE, $plugin) {
  $entity_type = $plugin['keyword'];
  $entity = entity_get_info($entity_type);

  $context = new ctools_context(array('entity:' . $entity_type, 'entity', $entity_type));
  $context->plugin = $plugin['name'];
  $context->keyword = $entity_type;

  if ($empty) {
    return $context;
  }

  $data = entity_load($entity_type, explode(',', $data));

  if (!empty($data)) {
    $context->data     = $data;
    if (!empty($entity['entity keys']['label'])) {
      $context->title    = t('A list of users');
    }

    if ($entity['entity keys']['bundle']) {
      $context->restrictions['type'] = array($data->{$entity['entity keys']['bundle']});
    }
    return $context;
  }

}

function acec_context_entities_settings_form($form, &$form_state) {
  $conf = &$form_state['conf'];
  $plugin = &$form_state['plugin'];

  $form['entity'] = array(
    '#title' => t('Enter the title or ID of a @entity entity', array('@entity' => $plugin['keyword'])),
    '#type' => 'textfield',
    '#maxlength' => 512,
    '#autocomplete_path' => 'ctools/autocomplete/' . $plugin['keyword'],
    '#weight' => -10,
  );

  if (!empty($conf['entity_id'])) {
    $info = entity_load($plugin['keyword'], array($conf['entity_id']));
    $info = $info[$conf['entity_id']];
    if ($info) {
      $entity = entity_get_info($plugin['keyword']);
      $uri = entity_uri($plugin['keyword'], $info);
      if (is_array($uri) && $entity['entity keys']['label']) {
        $link = l(t("'%title' [%type id %id]", array('%title' => $info->{$entity['entity keys']['label']}, '%type' => $plugin['keyword'], '%id' => $conf['entity_id'])), $uri['path'], array('attributes' => array('target' => '_blank', 'title' => t('Open in new window')), 'html' => TRUE));
      }
      elseif (is_array($uri)) {
        $link = l(t("[%type id %id]", array('%type' => $plugin['keyword'], '%id' => $conf['entity_id'])), $uri['path'], array('attributes' => array('target' => '_blank', 'title' => t('Open in new window')), 'html' => TRUE));
      }
      elseif ($entity['entity keys']['label']) {
        $link = l(t("'%title' [%type id %id]", array('%title' => $info->{$entity['entity keys']['label']}, '%type' => $plugin['keyword'], '%id' => $conf['entity_id'])), file_create_url($uri), array('attributes' => array('target' => '_blank', 'title' => t('Open in new window')), 'html' => TRUE));
      }
      else {
        $link = t("[%type id %id]", array('%type' => $plugin['keyword'], '%id' => $conf['entity_id']));
      }
      $form['entity']['#description'] = t('Currently set to !link', array('!link' => $link));
    }
  }

  $form['entity_id'] = array(
    '#type' => 'value',
    '#value' => $conf['entity_id'],
  );

  $form['entity_type'] = array(
    '#type' => 'value',
    '#value' => $plugin['keyword'],
  );

  $form['set_identifier'] = array(
    '#type' => 'checkbox',
    '#default_value' => FALSE,
    '#title' => t('Reset identifier to entity label'),
    '#description' => t('If checked, the identifier will be reset to the entity label of the selected entity.'),
  );

  return $form;
}

/**
 * Validate a node.
 */
function acec_context_entities_settings_form_validate($form, &$form_state) {
  // Validate the autocomplete
  if (empty($form_state['values']['entity_id']) && empty($form_state['values']['entity'])) {
    form_error($form['entity'], t('You must select an entity.'));
    return;
  }

  if (empty($form_state['values']['entity'])) {
    return;
  }

  $id          = $form_state['values']['entity'];
  $preg_matches = array();
  $match        = preg_match('/\[id: (\d+)\]/', $id, $preg_matches);
  if (!$match) {
    $match = preg_match('/^id: (\d+)/', $id, $preg_matches);
  }

  if ($match) {
    $id = $preg_matches[1];
  }
  if (is_numeric($id)) {
    $entity = entity_load($form_state['values']['entity_type'], array($id));
    $entity = $entity[$id];
  }
  else {
    $entity_info = entity_get_info($form_state['values']['entity_type']);
    $field = $entity_info['entity keys']['label'];
    $entity = entity_load($form_state['values']['entity_type'], FALSE, array($field => $id));
  }

  // Do not allow unpublished nodes to be selected by unprivileged users
  // || (empty($node->status) && !(user_access('administer nodes'))) need a new sanity check at some point.
  if (!$entity) {
    form_error($form['entity'], t('Invalid entity selected.'));
  }
  else {
    $entity_id = entity_extract_ids($form_state['values']['entity_type'], $entity);
    form_set_value($form['entity_id'], $entity_id[0], $form_state);
  }
}

function acec_context_entities_settings_form_submit($form, &$form_state) {
  if ($form_state['values']['set_identifier']) {
    $entity_info = entity_get_info($form_state['values']['entity_type']);
    $entity = entity_load($form_state['values']['entity_type'], array($form_state['values']['entity_id']));
    $entity = $entity[$form_state['values']['entity_id']];
    $form_state['values']['identifier'] = $entity->{$entity_info['entity keys']['label']};
  }

  // This will either be the value set previously or a value set by the
  // validator.
  $form_state['conf']['entity_id'] = $form_state['values']['entity_id'];
}

/**
 * Provide a list of ways that this context can be converted to a string.
 */
function acec_context_entities_convert_list($plugin) {
  $list = array();

  $entity = entity_get_info($plugin['context name']);
  if (isset($entity['token type'])) {
    $token = $entity['token type'];
  }
  else {
    $token = $plugin['context name'];
  }

  // Hack: we need either token.module or a core fix for this to work right,
  // until then, we just muscle it.
  if ($token == 'taxonomy_term') {
    $token = 'term';
  }

  $tokens = token_info();
  if (isset($tokens['tokens'][$token])) {
    foreach ($tokens['tokens'][$token] as $id => $info) {
      if (!isset($list[$id])) {
        $list[$id] = $info['name'];
      }
    }
  }
  return $list;
}

/**
 * Convert a context into a string.
 */
function acec_context_entities_convert($context, $type, $options = array()) {
  $entity_type = $context->type[2];
  $entity = entity_get_info($entity_type);

  if (isset($entity['token type'])) {
    $token = $entity['token type'];
  }
  else {
    $token = $entity_type;
  }

  // Hack: we need either token.module or a core fix for this to work right,
  // until then, we just muscle it.
  if ($token == 'taxonomy_term') {
    $token = 'term';
  }

  $tokens = token_info();

  $values = token_generate($token, array($type => $type), array($token => $context->data), $options);
  if (isset($values[$type])) {
    return $values[$type];
  }
}
