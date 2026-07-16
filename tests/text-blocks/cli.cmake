set(EXPORTED_MEI "${WORK_DIR}/text-blocks-cli.mei")
set(SVG_BASE "${WORK_DIR}/text-blocks-cli.svg")
file(REMOVE "${EXPORTED_MEI}")
file(GLOB OLD_SVG_PAGES "${WORK_DIR}/text-blocks-cli_*.svg")
if(OLD_SVG_PAGES)
    file(REMOVE ${OLD_SVG_PAGES})
endif()

execute_process(
    COMMAND "${VEROVIO}" -r "${RESOURCES}" -t mei -o "${EXPORTED_MEI}" "${FIXTURE}"
    RESULT_VARIABLE EXPORT_RESULT
)
if(NOT EXPORT_RESULT EQUAL 0 OR NOT EXISTS "${EXPORTED_MEI}")
    message(FATAL_ERROR "Verovio CLI did not export the text-block fixture")
endif()

file(READ "${EXPORTED_MEI}" EXPORTED_XML)
string(REGEX MATCHALL "tusk-syl-verse-2-line-[12]-[0-9][0-9][0-9]" EXTRACTED_SYLLABLES "${EXPORTED_XML}")
list(LENGTH EXTRACTED_SYLLABLES EXTRACTED_SYLLABLE_COUNT)
if(NOT EXTRACTED_SYLLABLE_COUNT EQUAL 92)
    message(FATAL_ERROR "CLI MEI export did not preserve exactly 92 extracted syllables")
endif()
if(NOT EXPORTED_XML MATCHES "<lg[^>]*xml:id=\"tusk-lg-verse-2\"")
    message(FATAL_ERROR "CLI MEI export did not preserve the later-verse line group")
endif()

# Rendering the exported file validates that the generated XML is parseable by
# the public CLI and produces usable SVG, rather than merely creating a file.
execute_process(
    COMMAND "${VEROVIO}" -r "${RESOURCES}" -a -o "${SVG_BASE}" "${EXPORTED_MEI}"
    RESULT_VARIABLE RENDER_RESULT
)
if(NOT RENDER_RESULT EQUAL 0)
    message(FATAL_ERROR "Verovio CLI could not re-import and render its MEI export")
endif()

file(GLOB SVG_PAGES "${WORK_DIR}/text-blocks-cli_*.svg")
list(SORT SVG_PAGES)
list(LENGTH SVG_PAGES SVG_PAGE_COUNT)
if(NOT SVG_PAGE_COUNT EQUAL 2)
    message(FATAL_ERROR "Expected the CLI fixture to render as exactly two SVG pages")
endif()

list(GET SVG_PAGES 0 FIRST_PAGE)
list(GET SVG_PAGES 1 SECOND_PAGE)
file(READ "${FIRST_PAGE}" FIRST_SVG)
file(READ "${SECOND_PAGE}" SECOND_SVG)
if(FIRST_SVG MATCHES "tusk-div-later-verse-2")
    message(FATAL_ERROR "Later-verse div rendered before the score had finished")
endif()
foreach(REQUIRED_ID tusk-div-later-verse-2 tusk-stack-verse-2-line-1-001 tusk-stack-verse-2-line-2-046)
    if(NOT SECOND_SVG MATCHES "${REQUIRED_ID}")
        message(FATAL_ERROR "Later-verse div was missing or split during CLI page cast-off: ${REQUIRED_ID}")
    endif()
endforeach()

# The fixture has a 2100px page with 50px margins (Verovio's SVG viewBox uses
# ten units per px), so translated drawing origins must remain at or left of
# the 20500-unit right content boundary.
string(REGEX MATCHALL "translate\\\(([0-9]+)" X_TRANSLATIONS "${SECOND_SVG}")
foreach(TRANSLATION ${X_TRANSLATIONS})
    string(REGEX REPLACE "translate\\\(" "" X "${TRANSLATION}")
    if(X GREATER 20500)
        message(FATAL_ERROR "CLI SVG content exceeded the fixture's right page boundary at x=${X}")
    endif()
endforeach()
