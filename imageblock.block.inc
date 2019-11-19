<?php

/**
 * BlockImage extends Block.
 *
 * This class allows us to create blocks with image and text.
 */
class BlockImage extends Block {

  /**
   * Constructor for an image block.
   */
  public function __construct($plugin_name, array $data) {
    parent::__construct($plugin_name, $data);

    $this->settings += array(
      'label' => '',
      'machine_name' => '',
    );
  }

  /**
   * Sets title text on draggable block panel in Layout builder.
   */
  public function getAdminTitle() {
    return !empty($this->settings['label']) ? check_plain($this->settings['label']) : t('Image block');
  }

  /**
   * Returns the rendered content of this block.
   *
   * @return string
   *   Rendered markup of an image block content.
   */
  public function getContent() {
    $elements = array(
      'content' => '',
      'image' => '',
      'wrapper_classes' => 'imageblock-inner',
    );

    $block = config_get('imageblock.block.' . $this->settings['machine_name']);
    if (!empty($block['content'])) {
      $content = token_replace($block['content']);
      $elements['content'] = check_markup($content, $block['format'], '', TRUE);
    }
    if (!empty($block['image'])) {
      $file = file_load($block['image']);
      $variables = array(
        'style_name' => 'thumbnail',
        'uri' => $file->uri,
        'width' => $file->width,
        'height' => $file->height,
        'alt' => $block['alt'],
        'attributes' => array(),
      );
      if (!empty($block['img_title'])) {
        $variables['attributes']['title'] = $block['img_title'];
      }

      $markup = theme('image', $variables);
      if (!empty($block['image_style'])) {
        $preset = image_style_load($block['image_style']);
        if ($preset) {
          $variables['style_name'] = $preset['name'];
          $markup = theme('image_style', $variables);
        }
      }
      // Wrap image with a link tag.
      if (!empty($block['link'])) {
        $attributes = array();
        $attributes['href'] = $block['link'];
        if (!empty($block['link_target'])) {
          $attributes['target'] = $block['link_target'];
        }
        if ($block['nofollow']) {
          $attributes['rel'] = 'nofollow';
        }
        $markup = '<a' . backdrop_attributes($attributes) . '>' . $markup . '</a>';
      }
      $elements['image'] = $markup;
      if (!empty($block['image_align'])) {
        switch ($block['image_align']) {
          case 'left':
            $elements['wrapper_classes'] .= ' imageblock-image-left clearfix';
            break;

          case 'right':
            $elements['wrapper_classes'] .= ' imageblock-image-right clearfix';
            break;

          case 'center':
            $elements['wrapper_classes'] .= ' imageblock-image-center';
            break;
        }
      }
    }
    return theme('imageblock_content', $elements);
  }

  /**
   * Builds the block's configuration form.
   */
  public function form(&$form, &$form_state) {
    parent::form($form, $form_state);
    if ($this->is_new) {
      $args = arg();
      if (isset($args[8])) {
        $block_info = explode(':', $args[8]);
        if (isset($block_info[1]) && $block_info[1] != 'default') {
          // Copy original data (image/text).
          $block = config_get('imageblock.block.' . $block_info[1]);
          // Force new machine name.
          $this->settings['machine_name'] = '';
        }
      }
    }
    else {
      // Edit existing block, no clone.
      $block = config_get('imageblock.block.' . $this->settings['machine_name']);
    }
    $form['label'] = array(
      '#type' => 'textfield',
      '#title' => t('Admin label'),
      '#default_value' => $this->settings['label'],
      '#maxlength' => 64,
      '#description' => t('This text is used only in administrative interfaces. It will not be shown to site visitors.'),
      '#required' => TRUE,
      '#id' => 'imageblock-label',
      '#weight' => -1,
    );
    $form['machine_name'] = array(
      '#type' => 'machine_name',
      '#default_value' => $this->settings['machine_name'],
      '#maxlength' => 128,
      '#machine_name' => array(
        'exists' => 'imageblock_load',
        'source' => array('label'),
      ),
      '#description' => t('A unique machine-readable name for this Block. It must only contain lowercase letters, numbers, and underscores.'),
      '#disabled' => !empty($this->settings['machine_name']),
    );

    // Simplify title settings as Image blocks have no default title.
    $form['title_display']['title_display']['#default_value'] = LAYOUT_TITLE_CUSTOM;
    $form['title_display']['title_display']['#access'] = FALSE;
    $form['title_display']['title']['#description'] = t('Leave empty for no block title.');
    // Add an image.
    $upload_validators = array('file_validate_extensions' => array('jpg jpeg png gif'));
    $config = config_get('imageblock.settings');
    if (!empty($config['imageblock_max_file_size'])) {
      $max_bytes = parse_size($config['imageblock_max_file_size']);
      $upload_validators['file_validate_size'] = array($max_bytes);
    }
    if (!empty($config['imageblock_max_dimensions'])) {
      // TODO file a core bug report reg. "undefined offset 1".
      $upload_validators['file_validate_image_resolution'] = array($config['imageblock_max_dimensions'], '0');
    }
    $upload_description = theme('file_upload_help', array(
      'upload_validators' => $upload_validators,
    ));
    $upload_location = 'public://';
    if (!empty($config['imageblock_image_path'])) {
      $upload_location .= $config['imageblock_image_path'];
    }
    $form['image'] = array(
      '#type' => 'managed_file',
      '#title' => t('Image'),
      '#description' => $upload_description,
      '#default_value' => isset($block['image']) ? $block['image'] : '',
      '#upload_location' => $upload_location,
      '#upload_validators' => $upload_validators,
    );
    // Additional image settings and attributes.
    $form['extended'] = array(
      '#type' => 'fieldset',
      '#title' => t('Additional image settings'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );
    $options = array();
    foreach (image_styles() as $key => $preset) {
      $options[$key] = $preset['name'];
    }
    if (!empty($options)) {
      $form['extended']['image_style'] = array(
        '#type' => 'select',
        '#title' => t('Image style'),
        '#options' => array('' => t('<none>')) + $options,
        '#default_value' => isset($block['image_style']) ? $block['image_style'] : '',
      );
    }
    $form['extended']['image_align'] = array(
      '#type' => 'select',
      '#title' => t('Image alignment'),
      '#options' => array(
        '' => t('None'),
        'left' => t('Left'),
        'right' => t('Right'),
        'center' => t('Center'),
      ),
      '#default_value' => isset($block['image_align']) ? $block['image_align'] : '',
    );
    $form['extended']['alt'] = array(
      '#type' => 'textfield',
      '#title' => t('Alternate text'),
      '#description' => t('This text will be used by screen readers, search engines, or when the image cannot be loaded.'),
      '#default_value' => isset($block['alt']) ? $block['alt'] : '',
    );
    $form['extended']['img_title'] = array(
      '#type' => 'textfield',
      '#title' => t('Title'),
      '#description' => t('The title is used as a tool tip when the user hovers the mouse over the image.'),
      '#default_value' => isset($block['img_title']) ? $block['img_title'] : '',
    );
    // FIXME find a better form item, "link" isn't usable with node/xx.
    $form['extended']['link'] = array(
      '#type' => 'url',
      '#title' => t('Link'),
      '#description' => t('Leave empty for no link.'),
      '#default_value' => isset($block['link']) ? $block['link'] : '',
    );
    $form['extended']['link_target'] = array(
      '#type' => 'select',
      '#title' => t('Link target'),
      '#options' => array(
        '' => 'none',
        '_blank' => '_blank',
        '_parent' => '_parent',
        '_self' => '_self',
        '_top' => '_top',
      ),
      '#default_value' => isset($block['link_target']) ? $block['link_target'] : '',
    );
    $form['extended']['nofollow'] = array(
      '#type' => 'checkbox',
      '#title' => t('NoFollow option'),
      '#description' => t('Mark this item if you want add rel=nofollow to your link.'),
      '#default_value' => !empty($block['nofollow']) ? $block['nofollow'] : FALSE,
    );
    $form['content'] = array(
      '#type' => 'text_format',
      '#title' => t('Block content'),
      '#default_value' => isset($block['content']) ? $block['content'] : '',
      '#format' => isset($block['format']) ? $block['format'] : NULL,
      '#editor_uploads' => TRUE,
      '#rows' => 5,
    );

  }

  /**
   * Validates the form settings.
   */
  public function formValidate($form, &$form_state) {
    parent::formValidate($form, $form_state);

    // Check that one of title, text, or image, is provided.
    if (empty($form_state['values']['image']) && empty($form_state['values']['title']) && empty($form_state['values']['content']['value'])) {
      form_error($form['image'], t('An image, a Display title, or Block content must be provided.'));
    }
  }

  /**
   * Submit handler to save the form settings.
   */
  public function formSubmit($form, &$form_state) {
    parent::formSubmit($form, $form_state);
    $this->settings['label'] = $form_state['values']['label'];
    $this->settings['machine_name'] = $form_state['values']['machine_name'];

    // Additional content goes to a different config.
    $block = array();
    $block['label'] = $form_state['values']['label'];
    $block['machine_name'] = $form_state['values']['machine_name'];
    $block['title'] = $form_state['values']['title'];
    $block['content'] = $form_state['values']['content']['value'];
    $block['format'] = $form_state['values']['content']['format'];
    // TODO file a core bug report for the hero block reg file usage!
    // @see also https://github.com/backdrop/backdrop-issues/issues/4209
    if (!empty($form_state['values']['image'])) {
      if (!$this->is_new) {
        // Edit an existing block.
        if ($form_state['values']['image'] != $form['image']['#default_value']) {
          $new_file = file_load($form_state['values']['image']);
          // Set file usage on new file, also marks permanent.
          file_usage_add($new_file, 'imageblock', 'block', $new_file->fid);
          if (!empty($form['image']['#default_value'])) {
            // Remove usage from old (replaced) file.
            $old_file = file_load($form['image']['#default_value']);
            file_usage_delete($old_file, 'imageblock');
          }
        }
      }
      else {
        // New block or copy of an existing block, increase file usage.
        $file = file_load($form_state['values']['image']);
        file_usage_add($file, 'imageblock', 'block', $file->fid);
      }
    }
    else {
      if (!empty($form['image']['#default_value'])) {
        // Old image was removed, no new one added.
        $old_file = file_load($form['image']['#default_value']);
        if ($old_file) {
          file_usage_delete($old_file, 'imageblock');
        }
      }
    }
    $block['image'] = $form_state['values']['image'];
    // Extended attributes.
    $block['alt'] = $form_state['values']['extended']['alt'];
    $block['img_title'] = $form_state['values']['extended']['img_title'];
    $block['link'] = $form_state['values']['extended']['link'];
    $block['link_target'] = $form_state['values']['extended']['link_target'];
    $block['nofollow'] = $form_state['values']['extended']['nofollow'];
    $block['image_style'] = $form_state['values']['extended']['image_style'];
    $block['image_align'] = $form_state['values']['extended']['image_align'];

    $config = config('imageblock.block.' . $block['machine_name']);
    $config->setData($block);
    $config->save();
  }

}