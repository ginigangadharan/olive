#include "preferencesdialog.h"

#include "io/config.h"

#include <QMenuBar>
#include <QAction>
#include <QVBoxLayout>
#include <QTabWidget>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QGroupBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QDialogButtonBox>
#include <QTreeWidget>
#include <QVector>
#include <QPushButton>
#include <QTreeWidgetItem>
#include <QList>
#include <QDoubleSpinBox>

#include "debug.h"

KeySequenceEditor::KeySequenceEditor(QWidget* parent, QAction* a)
	: QKeySequenceEdit(parent), action(a) {
	setKeySequence(action->shortcut());
	//connect(this, SIGNAL(editingFinished()), this, SLOT(set_action_shortcut()));
}

void KeySequenceEditor::set_action_shortcut() {
	action->setShortcut(keySequence());
}

PreferencesDialog::PreferencesDialog(QWidget *parent) :
	QDialog(parent)
{
	setWindowTitle("Preferences");
	setup_ui();

	accurateSeekButton->setChecked(!config.fast_seeking);
	fastSeekButton->setChecked(config.fast_seeking);
	recordingComboBox->setCurrentIndex(config.recording_mode - 1);
	imgSeqFormatEdit->setText(config.img_seq_formats);
}

PreferencesDialog::~PreferencesDialog() {}

void PreferencesDialog::setup_kbd_shortcut_worker(QMenu* menu, QTreeWidgetItem* parent) {
	QList<QAction*> actions = menu->actions();
	for (int i=0;i<actions.size();i++) {
		QAction* a = actions.at(i);

		if (!a->isSeparator()) {
			QTreeWidgetItem* item = new QTreeWidgetItem();
			item->setText(0, a->text().replace("&", ""));

			parent->addChild(item);

			if (a->menu() != NULL) {
				item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
				setup_kbd_shortcut_worker(a->menu(), item);
			} else {
				item->setData(0, Qt::UserRole + 1, reinterpret_cast<quintptr>(a));
				key_shortcut_items.append(item);
				key_shortcut_actions.append(a);
			}
		}
	}
}

void PreferencesDialog::setup_kbd_shortcuts(QMenuBar* menubar) {
	QList<QAction*> menus = menubar->actions();

	for (int i=0;i<menus.size();i++) {
		QMenu* menu = menus.at(i)->menu();

		QTreeWidgetItem* item = new QTreeWidgetItem();
		item->setText(0, menu->title().replace("&", ""));

		keyboard_tree->addTopLevelItem(item);

		setup_kbd_shortcut_worker(menu, item);
	}

	for (int i=0;i<key_shortcut_items.size();i++) {
		KeySequenceEditor* editor = new KeySequenceEditor(keyboard_tree, key_shortcut_actions.at(i));
		keyboard_tree->setItemWidget(key_shortcut_items.at(i), 1, editor);
		key_shortcut_fields.append(editor);
	}
}

void PreferencesDialog::save() {
	config.recording_mode = recordingComboBox->currentIndex() + 1;
	config.img_seq_formats = imgSeqFormatEdit->text();
	config.fast_seeking = fastSeekButton->isChecked();
	config.disable_multithreading_for_images = disable_img_multithread->isChecked();
	config.upcoming_queue_size = upcoming_queue_spinbox->value();
	config.upcoming_queue_type = upcoming_queue_type->currentIndex();
	config.previous_queue_size = previous_queue_spinbox->value();
	config.previous_queue_type = previous_queue_type->currentIndex();

	// save keyboard shortcuts
	for (int i=0;i<key_shortcut_fields.size();i++) {
		key_shortcut_fields.at(i)->set_action_shortcut();
	}

	accept();
}

void PreferencesDialog::reset_default_shortcut() {
	QList<QTreeWidgetItem*> items = keyboard_tree->selectedItems();
	for (int i=0;i<items.size();i++) {
		QTreeWidgetItem* item = keyboard_tree->selectedItems().at(i);
		const QVariant& data = item->data(0, Qt::UserRole + 1);
		if (!data.isNull()) {
			QAction* a = reinterpret_cast<QAction*>(data.value<quintptr>());
			QKeySequence ks(a->property("default").toString());
			static_cast<QKeySequenceEdit*>(keyboard_tree->itemWidget(item, 1))->setKeySequence(ks);
		}
	}
}

bool PreferencesDialog::refine_shortcut_list(const QString &s, QTreeWidgetItem* parent) {
	if (parent == NULL) {
		for (int i=0;i<keyboard_tree->topLevelItemCount();i++) {
			refine_shortcut_list(s, keyboard_tree->topLevelItem(i));
		}
	} else {
		parent->setExpanded(!s.isEmpty());

		bool all_children_are_hidden = !s.isEmpty();

		for (int i=0;i<parent->childCount();i++) {
			QTreeWidgetItem* item = parent->child(i);
			if (item->childCount() > 0) {
				all_children_are_hidden = refine_shortcut_list(s, item);
			} else {
				item->setHidden(false);
				if (s.isEmpty()) {
					all_children_are_hidden = false;
				} else {
					QString shortcut;
					if (keyboard_tree->itemWidget(item, 1) != NULL) {
						shortcut = static_cast<QKeySequenceEdit*>(keyboard_tree->itemWidget(item, 1))->keySequence().toString();
					}
					if (item->text(0).contains(s, Qt::CaseInsensitive) || shortcut.contains(s, Qt::CaseInsensitive)) {
						all_children_are_hidden = false;
					} else {
						item->setHidden(true);
					}
				}
			}
		}

		if (parent->text(0).contains(s, Qt::CaseInsensitive)) all_children_are_hidden = false;

		parent->setHidden(all_children_are_hidden);

		return all_children_are_hidden;
	}
}

void PreferencesDialog::setup_ui() {
	QVBoxLayout* verticalLayout = new QVBoxLayout(this);
	QTabWidget* tabWidget = new QTabWidget(this);

	QTabWidget* general_tab = new QTabWidget();
	QGridLayout* general_layout = new QGridLayout(general_tab);

	general_layout->addWidget(new QLabel("Image sequence formats:"), 0, 0, 1, 1);

	imgSeqFormatEdit = new QLineEdit(general_tab);

	general_layout->addWidget(imgSeqFormatEdit, 0, 1, 1, 1);

	general_layout->addWidget(new QLabel("Audio Recording:"), 1, 0, 1, 1);

	recordingComboBox = new QComboBox(general_tab);
	recordingComboBox->addItem("Mono");
	recordingComboBox->addItem("Stereo");

	general_layout->addWidget(recordingComboBox, 1, 1, 1, 1);

	tabWidget->addTab(general_tab, "General");
	QWidget* behavior_tab = new QWidget();
	tabWidget->addTab(behavior_tab, "Behavior");

	// Playback
	QWidget* playback_tab = new QWidget();
	QVBoxLayout* playback_tab_layout = new QVBoxLayout(playback_tab);

	// Playback -> Disable Multithreading on Images
	disable_img_multithread = new QCheckBox("Disable Multithreading on Images");
	disable_img_multithread->setChecked(config.disable_multithreading_for_images);
	playback_tab_layout->addWidget(disable_img_multithread);

	// Playback -> Seeking
	QGroupBox* seeking_group = new QGroupBox(playback_tab);
	seeking_group->setTitle("Seeking");
	QVBoxLayout* seeking_group_layout = new QVBoxLayout(seeking_group);
	accurateSeekButton = new QRadioButton(seeking_group);
	accurateSeekButton->setText("Accurate Seeking\nAlways show the correct frame (visual may pause briefly as correct frame is retrieved)");
	seeking_group_layout->addWidget(accurateSeekButton);
	fastSeekButton = new QRadioButton(seeking_group);
	fastSeekButton->setText("Fast Seeking\nSeek quickly (may briefly show inaccurate frames when seeking - doesn't affect playback/export)");
	seeking_group_layout->addWidget(fastSeekButton);
	playback_tab_layout->addWidget(seeking_group);

	// Playback -> Memory Usage
	QGroupBox* memory_usage_group = new QGroupBox(playback_tab);
	memory_usage_group->setTitle("Memory Usage");
	QGridLayout* memory_usage_layout = new QGridLayout(memory_usage_group);
	memory_usage_layout->addWidget(new QLabel("Upcoming Frame Queue:"), 0, 0);
	upcoming_queue_spinbox = new QDoubleSpinBox();
	upcoming_queue_spinbox->setValue(config.upcoming_queue_size);
	memory_usage_layout->addWidget(upcoming_queue_spinbox, 0, 1);
	upcoming_queue_type = new QComboBox();
	upcoming_queue_type->addItem("frames");
	upcoming_queue_type->addItem("seconds");
	upcoming_queue_type->setCurrentIndex(config.upcoming_queue_type);
	memory_usage_layout->addWidget(upcoming_queue_type, 0, 2);
	memory_usage_layout->addWidget(new QLabel("Previous Frame Queue:"), 1, 0);
	previous_queue_spinbox = new QDoubleSpinBox();
	previous_queue_spinbox->setValue(config.previous_queue_size);
	memory_usage_layout->addWidget(previous_queue_spinbox, 1, 1);
	previous_queue_type = new QComboBox();
	previous_queue_type->addItem("frames");
	previous_queue_type->addItem("seconds");
	previous_queue_type->setCurrentIndex(config.previous_queue_type);
	memory_usage_layout->addWidget(previous_queue_type, 1, 2);
	playback_tab_layout->addWidget(memory_usage_group);

	tabWidget->addTab(playback_tab, "Playback");

	QWidget* shortcut_tab = new QWidget();

	QVBoxLayout* shortcut_layout = new QVBoxLayout(shortcut_tab);

	QLineEdit* key_search_line = new QLineEdit();
	key_search_line->setPlaceholderText("Search for action or shortcut");
	connect(key_search_line, SIGNAL(textChanged(const QString &)), this, SLOT(refine_shortcut_list(const QString &)));

	shortcut_layout->addWidget(key_search_line);

	keyboard_tree = new QTreeWidget();
	QTreeWidgetItem* tree_header = keyboard_tree->headerItem();
	tree_header->setText(0, "Action");
	tree_header->setText(1, "Shortcut");
	shortcut_layout->addWidget(keyboard_tree);

	QHBoxLayout* reset_shortcut_layout = new QHBoxLayout();
	reset_shortcut_layout->addStretch();

	reset_shortcut_button = new QPushButton("Reset to Default");
	reset_shortcut_layout->addWidget(reset_shortcut_button);
	connect(reset_shortcut_button, SIGNAL(clicked(bool)), this, SLOT(reset_default_shortcut()));

	shortcut_layout->addLayout(reset_shortcut_layout);

	tabWidget->addTab(shortcut_tab, "Keyboard");

	verticalLayout->addWidget(tabWidget);

	QDialogButtonBox* buttonBox = new QDialogButtonBox(this);
	buttonBox->setOrientation(Qt::Horizontal);
	buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

	verticalLayout->addWidget(buttonBox);

	connect(buttonBox, SIGNAL(accepted()), this, SLOT(save()));
	connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

	tabWidget->setCurrentIndex(2);
}
