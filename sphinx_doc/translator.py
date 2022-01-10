import os
from googletrans import Translator

translator = Translator()

# translation = translator.translate("Hello", src="en", dest="fr")
# print(f"{translation.origin} ({translation.src}) --> {translation.text} ({translation.dest})")
file = 'niryo_robot_led_ring.po'
file2 = 'niryo_robot_led_ring.po'
folder_path = "/home/valentinp/catkin_ws/src/sphinx_doc/locale/fr/LC_MESSAGES/source/stack/high_level"
file_path = os.path.join(folder_path, file)
output_file_path = os.path.join(folder_path, file2)
trad_sentence = ""
read_trad = False


def split_str(sentance, char_nb_limit=60):
    splitted_sentence = []
    piece_of_sentence = ""
    for word in sentance.split():
        if len(piece_of_sentence) >= char_nb_limit:
            splitted_sentence.append('"{}"'.format(piece_of_sentence))
            piece_of_sentence = word
        elif len(piece_of_sentence) == 0:
            piece_of_sentence = word
        else:
            piece_of_sentence += ' ' + word

    splitted_sentence.append('"{}"'.format(piece_of_sentence))

    return '\n'.join(splitted_sentence)


overall_translation = ''
num_lines = sum(1 for line in open(file_path))

with open(file_path, 'r', encoding="utf-8") as source_file:
    cpt = 0
    while True:
        line = source_file.readline()
        cpt += 1
        if cpt % 50 == 0:
            print("{}/{}".format(cpt, num_lines))

        if not line:
            break

        if line.startswith('msgid'):
            trad_sentence = line.replace('msgid "', '').rstrip("\n").rstrip('"')
            read_trad = True
        elif line.startswith('msgstr'):
            read_trad = False
            if line.startswith('msgstr ""'):
                print(trad_sentence)
                translation = translator.translate(trad_sentence, dest="fr").text
                print(translation)
                overall_translation += 'msgstr {}\n'.format(split_str(translation))
                continue

        elif read_trad:
            trad_sentence += line.replace('"', '').rstrip("\n")

        overall_translation += line

with open(output_file_path, 'w', encoding="utf-8") as output_file:
    output_file.write(overall_translation)
