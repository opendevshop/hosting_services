<?php
/**
 * @file
 * Provide remote access to the signup module
 */

/**
 * Return the signup form
 */
function hosting_services_signup_form() {
  $form = drupal_get_form('hosting_signup_form');
  return $form;
}